<?php
#### reading the config file to get the nodes info

#$portsArray = array(7001, 7002, 7003, 7004);
$portsArray = array();
#$hostsArray = array("192.168.1.203", "192.168.1.203", "192.168.1.203", "192.168.1.203");
$hostsArray = array();

$config_file = fopen("cluster_config.txt", "r");
if($config_file){
         while (($line = fgets($config_file)) !== false) {
                if($line[0] == '#')continue;
                // process the line read.
                list($header, $tokens) = split('[|]' , $line);
                if($header == "ports"){
                        $portsArray = explode(' ', trim($tokens));
                        ## $portsStr to portsArray
                }else if ($header == "hostnames"){
                        $hostsArray = explode(' ', trim($tokens));
                        ## $portsStr to portsArray
                }       
         }
}else{
        echo "Config file is not available.";
        exit(0);
}

class Node{
        public $port;
        public $hostname;
}

$nodesArray = array();
for ($nodeIdx = 0; $nodeIdx < count($portsArray); ++$nodeIdx){
        $nodesArray[$nodeIdx] = new Node();
        $nodesArray[$nodeIdx]->port = $portsArray[$nodeIdx];
        $nodesArray[$nodeIdx]->hostname = $hostsArray[$nodeIdx];
}

#echo var_dump($nodesArray);
?>
