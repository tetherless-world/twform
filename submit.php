<?php

chdir("../../../../");
require_once("includes/bootstrap.inc");
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$return = menu_execute_active_handler();

$user = user_load($_POST["uid"]);
module_load_include("inc","node","node.pages");

function createDrupalPage($args) {
  global $user;
  $node = array("type" => "document");
  $form_state = array();
  $form_state['values'] = array();
  $form_state['values']['title'] = $args['title'];
  $form_state['values']['body'] = 
    "<document href=\"".$args["href"]."\" alt=\"".$args['title']."\"/>";
  $form_state['values']['uid'] = $user->uid;
  $form_state['values']['name'] = $user->name;
  $form_state['values']['status'] = 1;
  $form_state['values']['path'] = "doc/".preg_replace("/ /","_",$args['title']);
  $form_state['values']['op'] = t("Save");
  drupal_execute("document_node_form", $form_state, (object)$node);
  $errs = form_get_errors();
  if($errs && count($errs) > 0) {
    print_r($errs);
    return FALSE;
  }
  return $args['alias'];
}

if(!node_access("create","document")) {
  header("HTTP/1.1 403 Authorization Required");
  die;
}

$secret = twdocs_get_apikey();
$service = twdocs_get_service();
$request = "putFile";

$eol = "\r\n";
$mime_boundary = md5(time());

function field($name, $value) {
  global $eol, $mime_boundary;
  $ans = '';
  $ans .= "--$mime_boundary$eol";
  $ans .= "Content-Disposition: form-data; name=\"$name\"$eol$eol$value$eol";
  return $ans;
}

function filecontent($name, $fileName, $content) {
  global $eol, $mime_boundary;
  $ans = '';
  $ans .= "--$mime_boundary$eol";
  $ans .= "Content-Disposition: form-data; name=\"$name\"; filename=\"$fileName\"$eol";
  $ans .= "Content-Type: text/plain$eol";
  $ans .= "Content-Transfer-Encoding: base64$eol$eol";
  $ans .= chunk_split(base64_encode($content));
  $ans .= $eol;
  return $ans;
}

if(!isset($_POST["title"])||!isset($_POST["file"])) {
  echo "Missing parameters. Please check your query.";
  die;
}
if($_FILES["content"]["error"]!=UPLOAD_ERR_OK) {
  echo "File upload failed.";
  die;
}

$nonce = time();

$unhashed = $service.":".$nonce.":".$request.":".$secret;
$hashed = sha1($unhashed);

$data = "";

$data .= field("title",$_POST["title"]);
$data .= field("creator",$user->name);
$data .= field("service",$service);
$data .= field("request",$request);
$data .= field("file",$_POST["file"]);
$data .= field("nonce",$nonce);
$data .= field("hash",$hashed);

$content = @file_get_contents($_FILES["content"]["tmp_name"]);
if($content===FALSE) {
  echo "Unable to read uploaded file.";
  die;
}
$data .= filecontent("content",$_POST["file"],$content);

$data .= "--$mime_boundary--$eol$eol";

$params = array('http' => array('method' => 'POST',
				'header' => 'Content-Type: multipart/form-data; boundary='.$mime_boundary.$eol,
				'content' => $data));

$ctx = stream_context_create($params);
$response = file_get_contents(twdocs_get_apiurl(),false,$ctx);
if($response===FALSE) {
  echo "Media server failed";
  die;
}

$res2 = json_decode($response);
if($res2===NULL||!$res2->success) {
  echo "Invalid response from media server: $response";
  die;
}

$args = array("title"=>$_POST["title"],
	      "href"=>$_POST["file"]);
$str=createDrupalPage($args);
if($str) {
  header("Location: $str");
}

header("HTTP/1.1 500 Internal Server Error");
die;
