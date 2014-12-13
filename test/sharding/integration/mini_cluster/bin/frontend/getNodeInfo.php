<?php
include 'Nodes.php';
$ROOT="/frontend";
$pageWidth=1500;
/////////////////////////////////////////////////////// Main /////////////////////////////////////////////////////
// Get cURL resource
$curl = curl_init();
$hostName = $hostsArray[0];
$bodyOnly = False;

$node=$nodesArray[0];
if(isset($_GET['node-index']) && intval($_GET['node-index']) < count($nodesArray)){
	$node=$nodesArray[intval($_GET['node-index'])];
}
if(isset($_GET['bodyOnly'])){
	$bodyOnly = ($_GET['bodyOnly'] == "true");
}
// Set some options - we are passing in a useragent too here
curl_setopt_array($curl, array(
    CURLOPT_RETURNTRANSFER => 1,
    CURLOPT_URL => 'http://'.$node->hostname.':'.$node->port.'/_debug/stats',
    CURLOPT_USERAGENT => 'Codular Sample cURL Request'
));
// Send the request & save response to $resp
$resp = curl_exec($curl);
//if(curl_errno($curl) == 7)
//{
//    echo "ServerIsDown";
//}else if(curl_errno($curl) == 0){
//    echo $resp;
//}


#echo "My first PHP script!";
$returnResult = "";
if(curl_errno($curl) == 7 || ! $resp){
	$returnResult = "<h2>ERROR : This port is not responding.</h2>";
}else if(curl_errno($curl) != 0){
	$returnResult = "<h2>ERROR : Known error.</h2>";	
}
if($bodyOnly){
	if($returnResult != ""){
		//printTop();
		echo $returnResult;
	}else{
		printJsonResponse($resp);
		//printTop();
		printBody($resp, $node);
	}
}
// Close request to clear up some resources
curl_close($curl);
if($bodyOnly){	
        exit(0);
}

?>
<!DOCTYPE html>
<html>
<head>
<script src="<?php echo $ROOT; ?>/jquery-2.1.1.min.js" type="text/javascript" ></script>
<link href="<?php echo $ROOT; ?>/themes/1/tooltip.css" rel="stylesheet" type="text/css" />
<script src="<?php echo $ROOT; ?>/themes/1/tooltip.js" type="text/javascript" ></script>
<script src="<?php echo $ROOT; ?>/tree.jquery.js" type="text/javascript" ></script>
<link rel="stylesheet" href="<?php echo $ROOT; ?>/jqtree.css">
<script>
ROOT='/frontend';
function replaceAll(find, replace, str) {
  return str.replace(new RegExp(find, 'g'), replace);
}
function loadRawResponse(resJSString){
	//alert('hello');
	resJSString = replaceAll("'","\"", resJSString);	
	var JSONObject = JSON.parse(resJSString);
	//var myJSONText = JSON.stringify(JSONObject, "");
	window.alert(resJSString);
	/*var data = [
	    {
		label: 'node1', id: 1,
		children: [
		    { label: 'child1', id: 2 },
		    { label: 'child2', id: 3 }
		]
	    },
	    {
		label: 'node2', id: 4,
		children: [
		    { label: 'child3', id: 5 }
		]
	    }
	];*/
	/*$('#raw_content').tree({
		data: data//,
//		autoOpen: true
	});*/
	//document.getElementById("raw_content").innerHTML = myJSONText;
	
}
function refereshInfo() {
	var xmlhttp;
	if (window.XMLHttpRequest)
	  {// code for IE7+, Firefox, Chrome, Safari, Opera
	  xmlhttp=new XMLHttpRequest();
	  }
	else
	  {// code for IE6, IE5
	  xmlhttp=new ActiveXObject("Microsoft.XMLHTTP");
	  }
	xmlhttp.onreadystatechange=function(){
	  if (xmlhttp.readyState==4 && xmlhttp.status==200){
	    if(xmlhttp.responseText.indexOf("ERROR : This port is not responding.") > -1){
		if(document.getElementById("body_id").innerHTML.indexOf("ERROR : This port is not responding.") <= -1){
		    	document.getElementById("body_id").innerHTML = xmlhttp.responseText + 
						document.getElementById("body_id").innerHTML;
		}
	    }else{
		document.getElementById("body_id").innerHTML=xmlhttp.responseText;
	    }
	    setTimeout(refereshInfo, 3000);
	  }
	}
	xmlhttp.open("GET", ROOT+"/getNodeInfo.php?batch_operation=none&bodyOnly=true&node-index=" + $("#nodePortTagId option:selected").val(),true);
	//xmlhttp.open("GET","http://www.google.com",true);
	xmlhttp.send();
}


function runBatch(operation){
var xmlhttp;
	if (window.XMLHttpRequest){// code for IE7+, Firefox, Chrome, Safari, Opera
	    xmlhttp=new XMLHttpRequest();
	}else{// code for IE6, IE5
	    xmlhttp=new ActiveXObject("Microsoft.XMLHTTP");
	}
	xmlhttp.onreadystatechange=function(){
	  if (xmlhttp.readyState==4 && xmlhttp.status==200){
		if(xmlhttp.responseText == "batch"){
		    	$('#batch_form').css("opacity","1");
			$('#batch_submit').val("Run");
		}else if(xmlhttp.responseText == "save"){
			$('#save_form').css("opacity","1");
			$('#save_button').val("Save");
		}
	  }
	}
	if(operation == "batch"){
		var batch_operation = $("#batch_operation option:selected").val();
		var batch_idx = $('#batch_idx').val();
		$('#batch_idx').attr("title", $('#batch_idx').attr("title") + batch_idx + batch_operation);
		var batch_node_index = $("#nodePortTagId option:selected").val();
		var batch_core_name = $("#save_core_name option:selected").val();
		if(batch_idx == ""){
			alert("Batch file index is not entered.");
			return;	
		}
		xmlhttp.open("GET",ROOT+"/runBatch.php?batch_operation="+batch_operation+"&batch_index="+batch_idx+"&batch_node_index="+batch_node_index+"&batch_core_name="+batch_core_name,true);
		xmlhttp.send();
		$('#batch_form').css("opacity","0.5");
		$('#batch_submit').val("Wait");
	}else if(operation == "save"){
		var core_name = $("#save_core_name option:selected").val();
		xmlhttp.open("GET",ROOT+"/runBatch.php?save_core_name="+core_name+"&node-index="+$("#nodePortTagId option:selected").val(),true);
		xmlhttp.send();
		$('#save_form').css("opacity","0.5");
		$('#save_button').val("Wait");
	}
}

</script>
<style>
* {
	font-weight:bold;
	
}
div {
	border:solid;
	width: calc(100% - 15px);
}
.DATA{
	border:none;
}
.button_class{
	background-color:PaleVioletRed;
}

</style>
</head>

<body onload="refereshInfo()" style="width:<?php echo $pageWidth; ?>px"><!-- 700 pixel is the total width -->
<?php
printTop();
?>
<span id="body_id">
	<?php	
	if($returnResult != ""){
		echo $returnResult;
	}else{
		printJsonResponse($resp);
		printBody($resp, $node);
	}
	?>
</span>
</body>
</html>
<?php exit(0); ?>

<?php
function printShard($shardId, $isLocal, $indexSize, $location, $comments, $bgColor, $L, $M, $X ){
?>
	<div id="<?php echo $shardId; ?>" title="<?php echo $comments; ?>" style="padding:5px;width:70px;display:table-cell;background-color:<?php echo $bgColor; ?>;">

		<div class="DATA" id="shard_ID"><?php echo $shardId; ?></div>
		<hr>
		<div class="DATA" id="shard_SIZE_location"><?php echo ($isLocal ? $indexSize." r" : ($location > 30000 ? "-" : "Node ".$location)) ; ?>
		</div>					
		<div class="DATA" id="shard_options_id_<?php echo $shardId; ?>" style="border-top:dotted;display:table-cell;width:70px">
			<?php
				if($L != ""){
					$lockHolders = implode(',', $L);
				}else{
					$lockHolders = "";
				}
			?>
			<span id="shard_options_locked_id" style="<?php echo ($L == '' ?'visibility: hidden;':''); ?>margin-top:2px;color:purple" 
				title="<?php echo $lockHolders; ?>">
			<?php
				if($L != ""){
					echo $L[0];
				}
			?>			
			</span><!-- TODO -->
			-
			<span id="shard_options_locked_id" style="<?php echo ($M['set'] == true ?'visibility: hidden;':''); ?>margin-top:46px;color:purple" 
					title="Last merge time : <?php echo ($M['set'] ?'':$M['last_merge_time']); ?>">
			M
			</span><!-- TODO -->
		</div>
	</div>
<?php
}
?>



<?php 
function printTop(){
global $node;
global $nodesArray;
global $batch_port;
global $batch_operation;
global $batch_index;
?>
<div id="top-administrator" style="display:block;padding:5px;border-bottom-width:0">
	<div style="background-color:LightGray;border:none;display:table-cell">
		<button class="button_class" type="button" onclick="refereshInfo()">Referesh Info</button>
		<span>Node:</span>
		<select id="nodePortTagId" name="node-index" form="batch_form">
<?php
	for ($nodeIdx = 0 ; $nodeIdx < count($nodesArray); ++$nodeIdx){
                $nodeItr = $nodesArray[$nodeIdx];
		var_dump($nodeItr);
		if($nodeItr == $node){
			?><option selected="selected" 
			value="<?php echo $nodeIdx; ?>"><?php echo $nodeItr->port; ?></option><?php
		}else{
			?><option value="<?php echo $nodeIdx; ?>"><?php echo $nodeItr->port; ?></option><?php
		}
	}
?>
		</select>
	</div>
	<hr>
	<div id="save_form" style="background-color:LightGray;border:none;display:table-cell">
		<select name="save_core_name" id="save_core_name">
			<option selected="selected" value="stackoverflow">Stackoverflow</option>
			<option value="statemedia">Statemedia</option>
		</select>
	</div>
	<form id="batch_form" style="background-color:LightGray;border:none;display:table-cell;" >
		<select name="batch_operation" id="batch_operation">
			<option  selected="selected" id="batch_insert" value="insert">Insert</option>
			<option id="batch_delete" value="delete">Delete</option>
		</select>
		File index:<input id="batch_idx" type="text" name="batch_index" 
					value="<?php echo $batch_index; ?>" size="1" title="">
	</form>
	<span style="display:table-cell;"><input id="save_button" class="button_class" type="button" onclick="runBatch('save');" value="Save" style="display:table-cell"/></span>
	<span style="display:table-cell"><input id="batch_submit" class="button_class" type="button" name="batch_submit" value="Run" onclick="runBatch('batch');" style="display:table-cell"/></span>
	<!-- <span style="margin-left:2px">|</span>
	<button id="save_button" type="button" onclick="runBatch()">Save</button>-->
</div>
<?php
}
?>

<?php
function printBody($resp, $nodeInfoObj){
$resp = str_replace("-" , "_" , $resp);
$jsonData = json_decode($resp);
if(isset($jsonData->error)){
	echo $jsonData->error;
	return;
}
$metadata_lock_holders = array();
if(property_exists($jsonData, 'metadata_locks')){
	if(property_exists($jsonData->metadata_locks, 'srch2_cluster_metadata')){
		$metadata_locks = $jsonData->metadata_locks->srch2_cluster_metadata->lock_holders;
		for($i = 0 ; $i < count($metadata_locks); $i++ ){
			array_push($metadata_lock_holders, substr($metadata_locks[$i]->holder, 1, strpos($metadata_locks[$i]->holder, ',')-1));
		}
	}
}
?>


<div id="top-administrator" style="display:block;padding:5px;border-bottom-width:0px">

	<div style="border:none">Cluster nodes :</div>
	<!--<div id="node_container" style="display:block;padding:5px">-->
<?php 
		$nodesInfo = $jsonData->nodes_info;
		for($nodeIdx = 0 ; $nodeIdx < count($nodesInfo); $nodeIdx++){
			$bgColor="Brown";
			$ID = $nodesInfo[$nodeIdx]->ID;
			$NodeInfoAvailable = True;
			$titleValue = "";
			if(property_exists($nodesInfo[$nodeIdx], 'obj')){
				$bgColor="LightBlue";
				$NodeInfoAvailable = False;			
			}else{
				$State = $nodesInfo[$nodeIdx]->State;

				if($State == "NOT_ARRIVED"){
					$bgColor="white";
				}else if($State == "ARRIVED"){
					$bgColor="Khaki";
				} else if($State == "FAILED"){
					$bgColor="Tomato";
				} else {
					echo "ERROR : wrong state value : " . $State;
				}
				if(in_array($ID, $metadata_lock_holders)){
					$bgColor = "LightGray";
					$titleValue .= "Has metadata lock.";
				}
				if($State != "FAILED"){
					$IP = $nodesInfo[$nodeIdx]->IP;
					$Port = $nodesInfo[$nodeIdx]->Port;
					$isMe = ($nodesInfo[$nodeIdx]->ME == "YES");
					$NodeName = $nodesInfo[$nodeIdx]->NodeName;
					if($isMe){
						$bgColor="LightGreen";
					}
				}
			}
?>
		   	<div id="node_template_id" style="background-color:<?php echo $bgColor; ?>;width:70px;display:table-cell;padding:2px;"
						title="<?php echo $titleValue; ?>" >
				<div class="DATA" id="node_port_id"><?php echo $ID." ".($NodeInfoAvailable && $State == "FAILED"? "" : "| ".$Port); ?></div>
				<?php if($NodeInfoAvailable && $State != "FAILED"){?>
				<hr>
				<div class="DATA" id="node_name"><?php echo $NodeName; ?></div>
				<?php }?>
			</div>
<?php
			if($nodeIdx > 0 && $nodeIdx % 9 == 8){
?>
				<div style="border:none;height:3px;display:block;"></div> <!-- going to next cell in a new row -->				
<?php
			}else{
?>
				<div style="width:2px;display:table-cell;border:none"></div> <!-- going to next cell in same row -->
<?php			
			}
		}

 ?>

	<!--</div>-->
</div>

<div id="core_info" style="display:block;padding:5px">
<?php
		if(property_exists($jsonData, 'primary_key_locks')){
			$primaryKeyLocks = addPrimaryKeyLocksInfo($jsonData->primary_key_locks);          
		}
		if(property_exists($jsonData, 'cluster_shard_locks')){
			$clusterShardLocks = addClusterShardLocks($jsonData->cluster_shard_locks);		
		}
		$clusterShardsInfo = $jsonData->cluster_shards;
		$nodeShardsInfo = $jsonData->node_shards;
		for($coreIdx = 0 ; $coreIdx < count($clusterShardsInfo); $coreIdx++){
			$clusterShardsArray = $clusterShardsInfo[$coreIdx]->cluster_shards;
			$coreName = $clusterShardsInfo[$coreIdx]->core_name;
			$distributed = ($clusterShardsInfo[$coreIdx]->distributed == "YES");
			$isAclCore = ($clusterShardsInfo[$coreIdx]->is_acl_core == "YES");
			if(! $distributed){
				continue;
			}
			$numReplicaShards = $clusterShardsInfo[$coreIdx]->num_replica_shards + 1;
			$numPrimaryShards = $clusterShardsInfo[$coreIdx]->num_primary_shards;

?>
			<div style="border:none"><?php echo $coreName; ?></div>
			<div id="core_<?php echo $coreName; ?>_id" style="border-width:0px;padding:5px;<?php if($isAclCore) echo 'background-image:LightGray;' ?>">
<?php
			for($shardIdx = 0 ; $shardIdx < count($clusterShardsArray); $shardIdx++){
				$ID = $clusterShardsArray[$shardIdx]->ID;
				$CoreId = substr($ID, 1, strpos($ID, '_') - 1);
				$tmp = substr($ID, strpos($ID, '_') + 1);
				$PartitionId = substr($tmp, 0, strpos($tmp, '_'));
				$tmp = substr($tmp, strpos($tmp, '_') + 1);
				$ReplicaId = $tmp;			
				if($shardIdx % $numReplicaShards == 0){
					if($shardIdx > 0){
						?></div><?php
					}
					?>
                                        <!-- <div style="width:2px;display:table-cell;border:none"></div>-->
					<div id="<?php echo 'C'.$CoreId.'_'.$PartitionId.'_0' ?>" style="border:none;margin:0px;padding:0px;width:80px;display:inline-block">
						<div id="<?php echo 'options_C'.$CoreId.'_'.$PartitionId.'_0' ?>" style="padding:5px;width:70px;display:table-cell;">
					<?php
					if(isset($primaryKeyLocks)){
								$pkComments = "";
								if(array_key_exists('C'.$CoreId.'_'.$PartitionId.'_0', $primaryKeyLocks)){
									$pk_visibility = 'visible;';
									$pkComments = implode(',' , $primaryKeyLocks['C'.$CoreId.'_'.$PartitionId.'_0']);
								}else $pk_visibility = 'hidden;';
					?>
							<span name="primary_key_locks" class="tooltip" onmouseover="tooltip.pop(this, 'this is a text');" 
								style="visibility: <?php echo $pk_visibility;?>;margin-top:2px;color:red"
								title = "<?php echo $pkComments ?>">
								P
							</span>
							-
							<span id="shard_options_locked_id" style="visibility: hidden;margin-top:24px;" title="Merge flag set to off">
								M
							</span><!-- TODO -->
							-
							<span id="shard_options_locked_id" style="visibility: hidden;margin-top:46px;" title="X happend">
								X
							</span><!-- TODO -->
					<?php
					}else{
					?>
							<span style="visibility: hidden;margin-top:24px;" >
								P
							</span>
							-
							<span id="shard_options_locked_id" style="visibility: hidden;margin-top:24px;" title="Merge flag set to off">
								M
							</span><!-- TODO -->
							-
							<span id="shard_options_locked_id" style="visibility: hidden;margin-top:46px;" title="X happend">
								X
							</span><!-- TODO -->
					<?php
					}
					?>
						</div>
						<div style="border:none;height:3px;display:block;"></div> <!-- going to next cell in a new row -->
                                        <?php
				}


				$location = $clusterShardsArray[$shardIdx]->location;
				$isLocal = ($clusterShardsArray[$shardIdx]->local == "YES");
				$numberOfRecords = 0;
				$mergeInfo = array();
				$mergeInfo['set'] = true;
				$comments = "";
				if($isLocal){
					$Index_Dir = $clusterShardsArray[$shardIdx]->index_directory;
					$Json_File = $clusterShardsArray[$shardIdx]->json_file_path;
					$numberOfRecords = $clusterShardsArray[$shardIdx]->num_records;
					$mergeInfo['set'] = $clusterShardsArray[$shardIdx]->merge_flag;
					$mergeInfo['last_merge_time'] = $clusterShardsArray[$shardIdx]->last_merge_time;
					$comments = "(1|index dir)".$Index_Dir. ", (2| json file path)". $Json_File;
				}
				
				$load = $clusterShardsArray[$shardIdx]->load;
				$State = $clusterShardsArray[$shardIdx]->State;
				$bgColor = "white";
				$shardSizeLocation = "-";
				switch($State){
				case "UNASSIGNED":
					$bgColor = "white";
					break;
				case "PENDING":
					$bgColor = "orange";
					break;	
				case "READY":
					$bgColor = "Khaki";
					$shardSizeLocation = "Node ".$location;
					if($isLocal){
						$bgColor = "LightGreen";
					}
					break;
				}
				
?>

<?php
				if(isset($clusterShardLocks) && array_key_exists($ID, $clusterShardLocks)){
					printShard($ID, $isLocal, $numberOfRecords, $location, $comments, $bgColor, $clusterShardLocks[$ID], $mergeInfo, "" );					
				}else{
					printShard($ID, $isLocal, $numberOfRecords, $location, $comments, $bgColor, "", $mergeInfo, "" );
				}
?>
				<div style="border:none;height:3px;display:block;"></div> <!-- going to next cell in a new row -->	
<?php
			} // for on cluster shards
?>
			</div>
	</div>
	<hr>
	<?php } // for on cores 

	      for($coreIdx = 0 ; $coreIdx < count($clusterShardsInfo); $coreIdx++){
			$clusterShardsArray = $clusterShardsInfo[$coreIdx]->cluster_shards;
			$coreName = $clusterShardsInfo[$coreIdx]->core_name;
			$distributed = ($clusterShardsInfo[$coreIdx]->distributed == "YES");
			$isAclCore = ($clusterShardsInfo[$coreIdx]->is_acl_core == "YES");
			if($distributed){
				continue;
			}
			// now, only for node shards, grouped by core-name
?>
			<div style="border:none"><?php echo $coreName; ?></div>
			<div id="core_<?php echo $coreName; ?>_id" style="border-width:0px;padding:5px;<?php if($isAclCore) echo 'background-image:LightGray;' ?>">
<?php
			for($shardIdx = 0 ; $shardIdx < count($nodeShardsInfo); $shardIdx++){
				$nodeShard = $nodeShardsInfo[$shardIdx];
				$CoreId = $nodeShard->core_id;
                                if($coreName != $nodeShard->core_name){
					continue;
				}
				$PartitionId = $nodeShard->partition_id;
				$isLocal = ($nodeShard->local == "YES");
				$location = $nodeShard->location;
				$comments = "";
				$bgColor = "Khaki";
				$numRecords = 0;
				$mergeInfo = array();
				$mergeInfo['set'] = true;
                                if($isLocal){
					$indexDir = $nodeShard->index_directory;
					$jsonFilePath = $nodeShard->json_file_path;
					$numRecords = $nodeShard->num_records;
					$comments = "(1|index dir)".$indexDir. ", (2| json file path)". $jsonFilePath;
					$mergeInfo['set'] = $clusterShardsArray[$shardIdx]->merge_flag;
					$mergeInfo['last_merge_time'] = $clusterShardsArray[$shardIdx]->last_merge_time;
					$bgColor = "LightGreen";
				}
				$ID = "N".$location."_C".$CoreId."_P".$PartitionId;
				printShard($ID, $isLocal, $numRecords, $location, $comments, $bgColor, "", $mergeInfo, "" );
				if($shardIdx > 0 && $shardIdx % 9 == 8){ ?>
				<div style="border:none;height:3px;display:block;"></div> <!-- going to next cell in a new row -->	
				<?php }else{ ?>
				<div style="width:2px;display:table-cell;border:none"></div> <!-- going to next cell in same row -->
				<?php }
			}
?>
			</div>
<?php
	      } // for on cores

?>
</div>

<?php
	if(property_exists($jsonData, 'pending_lock_requests')){
		$pendingLockRequests = $jsonData->pending_lock_requests;
		addPendingLockRequests($pendingLockRequests);
	}
}

function printJsonResponse($resp){
	$respString = json_encode($resp);
	$respString = str_replace("-" , "_" , $respString);
	//$respString = str_replace("\\" , "" , $respString);
	$respString = str_replace("\\n" , "" , $respString);
	$respString = str_replace("\"" , "'" , $respString);
?>
<div id="raw_content" style="display:block;padding:5px;background-color:LightBlue;border-bottom-width:0" onclick="loadRawResponseMain();">
	<center>Click here to see the raw response.</center>
</div>
<script>
function loadRawResponseMain(){
	var input = <?php echo "\"".substr($respString, 1, strlen($respString)-2)."\"" ?>;
	loadRawResponse(input);
}
</script>

<?php
}


?>
//

<?php
function addPrimaryKeyLocksInfo($primary_key_locks){
	$partitionPrimaryKeyLocks;
	foreach($primary_key_locks as $primary_key=>$pk_value){
		$ClusterPID = $pk_value->containing_cluster_pid;
		$lockHoldersList = "";
		for($h = 0 ; $h < count($pk_value->lock_holders); $h++){
			$holderInfo = $pk_value->lock_holders[$h];		
			$holderNodeId = substr($holderInfo->holder, 1, strpos($holderInfo->holder, ',') - 1);
			if($h > 0) $lockHoldersList .= ",";
			$lockHoldersList .= $holderNodeId;
		}
		$lockHoldersList .= "has".$primary_key;
		if(property_exists($partitionPrimaryKeyLocks, $ClusterPID)){
			array_push($partitionPrimaryKeyLocks[$ClusterPID], $lockHoldersList);
		}else{
			$partitionPrimaryKeyLocks[$ClusterPID] = array();
			array_push($partitionPrimaryKeyLocks[$ClusterPID] , $lockHoldersList);
		}
	}
	return $partitionPrimaryKeyLocks;
}
?>

<?php
function addClusterShardLocks($clusterShardLocks){
	$result;
	foreach($clusterShardLocks as $ID => $lockHolders){
		if(count($lockHolders->lock_holders) > 0){
			$result[$ID] = array();
			for($i = 0 ; $i < count($lockHolders->lock_holders); $i++){
				$holderObj=$lockHolders->lock_holders[$i];
				$NodeId = substr($holderObj->holder, 1, strpos($holderObj->holder, ',')-1);
				$lockType = $holderObj->lock;
				array_push($result[$ID], $NodeId.$lockType);
			}
		}
	}
	return $result;
}
?>

<?php
function addPendingLockRequests($pendingLockRequests){
?>
<div style="border:none">Pending Lock Requests :</div>
<div id="node_container" style="display:block;padding:5px">
<?php
	for($i = 0 ; $i < count($pendingLockRequests); $i++){
		$type = $pendingLockRequests[$i]->batch_type;
		$blocking = $pendingLockRequests[$i]->blocking == "true";
		$incremental = $pendingLockRequests[$i]->incremental == "true";
		$release = $pendingLockRequests[$i]->release == "true";
		$requester = $pendingLockRequests[$i]->requester;
		$resources = $pendingLockRequests[$i]->resources;
		$resourcesDump = "";
		foreach($resources as $resKey => $resValue){
			if($resKey == 'metadata'){
				//$resourcesDump .= 'metadata: ';
			}else if($resKey == 'older_nodes'){
				$resourcesDump .= "older nodes:".implode('|', $resValue);
			}else if($resKey == 'containing_cluster_pid'){
				$resourcesDump .= 'cluster_pid:' . $resValue. "<br/>";
			}else if($resKey == 'primary_keys'){
				$resourcesDump .= 'primary_keys:' . $resValue;
			}
		}
		$bgColor = "White";
		switch($type){
		case 'metadata':
			$bgColor = "LightGray";
			break;
		case 'pk':
			$bgColor = "purple";
			break;
		case 'copy':
			$bgColor = "LightBlue";
			break;
		case 'move':
			$bgColor = "LightGreen";
			break;
		case 'gen_purpose':
			$bgColor = "LightYellow";
			break;
		case 'shard_list':
			$bgColor = "Orange";
			break;
		}
?>
	   	<div id="node_template_id" style="background-color:<?php echo $bgColor; ?>;width:70px;display:table-cell;padding:2px;"
					title="<?php echo $resourcesDump; ?>" >
			<?php 
				echo "From node ".$requester;
				echo "<hr/>";
				echo $type."<br/>".($blocking?"B":"NonB")."|".($release?"R":"L")."".($incremental?"|I":""); 
				echo "<hr/>";
				if($type == "metadata"){
					echo substr($resourcesDump, strpos($resourcesDump, "older nodes:")+12);
				}else if($type == "pk"){
					echo $resourcesDump;
				}
			?>
		</div>
<?php
		if($i > 0 && $i % 9 == 8){
?>
			<div style="border:none;height:3px;display:block;"></div> <!-- going to next cell in a new row -->				
<?php
		}else{
?>
			<div style="width:2px;display:table-cell;border:none"></div> <!-- going to next cell in same row -->
<?php			
		}
	}

?>
</div>

<?php
}
?>
