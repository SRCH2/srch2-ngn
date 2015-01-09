#include "Srch2ServerRuntime.h"


///* Convert an amount of bytes into a human readable string in the form
// * of 100B, 2G, 100M, 4K, and so forth.
// * Thanks Redis */
//void bytesToHuman(char *s, unsigned long long n) {
//	double d;
//
//	if (n < 1024) {
//		/* Bytes */
//		sprintf(s, "%lluB", n);
//		return;
//	} else if (n < (1024 * 1024)) {
//		d = (double) n / (1024);
//		sprintf(s, "%.2fK", d);
//	} else if (n < (1024LL * 1024 * 1024)) {
//		d = (double) n / (1024 * 1024);
//		sprintf(s, "%.2fM", d);
//	} else if (n < (1024LL * 1024 * 1024 * 1024)) {
//		d = (double) n / (1024LL * 1024 * 1024);
//		sprintf(s, "%.2fG", d);
//	}
//}

std::string getCurrentVersion() {
	return Version::getCurrentVersion();
}
void printVersion() {
	std::cout << "SRCH2 server version:" << getCurrentVersion() << std::endl;
}

void parseProgramArguments(int argc, char** argv,
		po::options_description& description,
		po::variables_map& vm_command_line_args) {
	description.add_options()("config-file",po::value<string>(), "Path to the config file")
            		("help", "Prints help message")
            		("version", "Prints version number of the engine");
	try {
		po::store(po::parse_command_line(argc, argv, description),
				vm_command_line_args);
		po::notify(vm_command_line_args);
	} catch (exception &ex) {
		cout << "error while parsing the arguments : " << endl << ex.what()
                		<< endl;
		cout << "Usage: <SRCH2_HOME>/bin/srch2-engine" << endl;
		cout << description << endl;
		exit(-1);
	}
}


/*
 * Returns 1 if everything is all right and we can continue.
 */

int parseConfigFilePath(int argc, char** argv, string & srch2_config_file){
	if (argc > 1) {
		if (strcmp(argv[1], "--version") == 0) {
			printVersion();
			return 0;
		}
	}
	// Parse command line arguments
	po::options_description description("Valid Arguments");
	po::variables_map vm_command_line_args;
	parseProgramArguments(argc, argv, description, vm_command_line_args);

	if (vm_command_line_args.count("help")) {
		cout << "Usage: <SRCH2_HOME>/bin/srch2-engine" << endl;
		cout << description << endl;
		return 0;
	}

	srch2_config_file = "";
	if (vm_command_line_args.count("config-file")) {
		srch2_config_file = vm_command_line_args["config-file"].as<string>();
		int status = ::access(srch2_config_file.c_str(), F_OK);
		if (status != 0) {
			std::cout << "config file = '" << srch2_config_file
					<< "' not found or could not be read" << std::endl;
			return -1;
		}
	} else {
		cout << "Usage: <SRCH2_HOME>/bin/srch2-engine" << endl;
		cout << description << endl;
		exit(-1);
	}
	return 1;
}


void initializeLogger(ConfigManager * serverConf){
	string logDir = getFilePath(serverConf->getHTTPServerAccessLogFile());
	// If the path does't exist, try to create it.
	if (!logDir.empty() && !checkDirExistence(logDir.c_str())) {
		if (createDir(logDir.c_str()) == -1) {
			Logger::error("Could not create log file %s.",
					serverConf->getHTTPServerAccessLogFile().c_str());
			exit(1);
		}
	}
	FILE *logFile = fopen(serverConf->getHTTPServerAccessLogFile().c_str(),
			"a");
	if (logFile == NULL) {
		Logger::setOutputFile(stdout);
		Logger::error("Open Log file %s failed.",
				serverConf->getHTTPServerAccessLogFile().c_str());
	} else {
		Logger::setOutputFile(logFile);
	}
	Logger::setLogLevel(serverConf->getHTTPServerLogLevel());

	//TODO to remove
	srch2::util::Logger::setLogLevel(srch2::util::Logger::SRCH2_LOG_DEBUG);
}

int main(int argc, char** argv) {

	srch2::httpwrapper::Srch2ServerRuntime * runtime =
			srch2::httpwrapper::Srch2ServerRuntime::getInstance();
	/*****************
	 * Constructing the ConfigManager
	 */
	string srch2_config_file;
	int parseOutput = parseConfigFilePath(argc, argv, srch2_config_file);
	if(parseOutput != 1){
		return parseOutput;
	}
	runtime->initializeConfigManager(srch2_config_file);

	/*****************
	 * Constructing the ShardManager
	 */
	// The writeview is not initialized yet at this point
	runtime->initializeShardManager();

	/*****************
	 * Loading the config file.
	 */
	if(! runtime->getConfigManager()->loadConfigFile(runtime->getMetadataManager())){
		Logger::error("Error in loading the config file, therefore exiting.");
		exit(-1);
	}

	/*
	 *  Check whether the license file is valid
	 */

	LicenseVerifier::testFile(runtime->getConfigManager()->getLicenseKeyFileName());


	/*
	 *  Check whether another process is holding the lock on node-name
	 */

	if(runtime->getConfigManager()->getCurrentNodeName().compare("") == 0){
		Logger::error("error: Node name is not usable.");
		exit(-1);
	}
	if(! runtime->getConfigManager()->tryLockNodeName()){
		Logger::error("error: Node name is not usable. Another instance is running with the same node name.");
		exit(-1);
	}


	/*****************
	 * Setup the Logger.
	 */
	initializeLogger(runtime->getConfigManager());

	/*****************
	 * Loading the data and initializing the metadata.
	 * Note : if there are json files given in config file, they will be
	 *        loaded here.
	 */
	srch2http::MetadataInitializer nodeInitializer(runtime->getConfigManager(), runtime->getMetadataManager());
	nodeInitializer.initializeNode();


	/*****************
	 * Initialize HTTP servers and internal communication passages
	 */
	int initOutput = runtime->initializeHttpServerMetadata();
	if (initOutput != 0) {
		Logger::close();
		return initOutput; // startup failed
	}

	initOutput = runtime->initializeThreadsAndServers();
	if (initOutput != 0) {
		Logger::close();
		return initOutput; // startup failed
	}


	/*****************
	 * Initializing TransportManager
	 */
	runtime->initializeTransportManager();


	/****************
	 * Initializing MigrationManager and attaching ShardManager to TM
	 */
	runtime->getShardManager()->attachToTransportManager(runtime->getTransportManager());
	runtime->getShardManager()->initMigrationManager();


	/****************
	 * Start internal communication threads
	 */
	initOutput = runtime->startInternalCommunication();
	if (initOutput != 0) {
		Logger::close();
		return initOutput; // startup failed
	}

	/****************
	 *  Start Synchronization manager thread. It performs following operation.
	 *  1. Start discovery thread to find existing cluster and join it. If there is no cluster
	 *     then this node becomes master.
	 *  2. Once the node joins the cluster, SM will use TM to setup a connection with master, so
	 *     that the connection can be used for inter-node communications.
	 *  3. SM get cluster info from master and setup connection with all the node in master.
	 */
	runtime->initializeSyncManager();
	// Discovery of SyncManager adds {currentNodeId, list of nodes} to the writeview.
	// we must start the ShardManager thread here before SM even has a chance to call it to
	// not miss any notifications from SM.
	runtime->getSyncManager()->startDiscovery();
	// at this point, node information is also available. So we can safely notify shard manager
	// that you have all the initial node information and can start distributed initialization.

	/*************
	 * All modules are ready, start shard manager, start syncManager and prepare data processor units
	 */
	runtime->getShardManager()->start();

	runtime->startSynchManagerThread();

	runtime->initializeDataProcessorUnits();


	/*************
	 * Open external HTTP channels to accept requests.
	 */
	runtime->openExternalChannels();


	/**************
	 * Initialize and start heart beat thread
	 */
	runtime->startSrch2ServerHeartBeat(); //TODO : Heartbeat flag doesn't have a lock, is it safe ?


	// wait in this process main thread until kill thread is handled ...
	runtime->waitForKillSignal();

	// exit gracefully: delete objects
	runtime->gracefulExit();

	return EXIT_SUCCESS;
}
