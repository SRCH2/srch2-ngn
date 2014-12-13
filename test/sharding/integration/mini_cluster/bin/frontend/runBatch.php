<?php
if(! isset($nodesArray)){
include 'Nodes.php';
}
if(isset($_GET["save_core_name"])){
$curl = curl_init();
// Set some options - we are passing in a useragent too here
curl_setopt_array($curl, array(
    CURLOPT_RETURNTRANSFER => 1,
    CURLOPT_URL => 'http://localhost:'.$_GET["node-index"].'/'.$_GET["save_core_name"].'/save',
    CURLOPT_USERAGENT => 'Codular Sample cURL Request',
    CURLOPT_CUSTOMREQUEST=> "PUT"
));
// Send the request & save response to $resp
$resp = curl_exec($curl);

if(curl_errno($curl) != 0){
	exit(0);
}
curl_close($curl);
echo "save";
exit(0);
}


$batch_node_index = $_GET["batch_node_index"];
$batch_operation = $_GET["batch_operation"];
$batch_index = $_GET["batch_index"];
$batch_core_name = $_GET["batch_core_name"];

$output = shell_exec('sh ./request_batches/'.$batch_core_name.'/'.$nodesArray[$batch_node_index]->port.'-'.$batch_operation.'-'.$batch_core_name.'-1000-'.$batch_index.'.sh');
echo "batch";
//echo 'sh ./request_batches/'.$batch_core_name.'/'.$batch_port.'-'.$batch_operation.'-'.$batch_core_name.'-1000-'.$batch_index.'.sh';

?>
