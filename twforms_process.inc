<?php

function twforms_get_timezone($dt = NULL, $tz = NULL) {
  if($tz == NULL) $tz = date_default_timezone_get();
  if($dt == NULL) $dt = "now";
  $here = new DateTimeZone($tz);
  $date = new DateTime($dt,$here);
  $hour = floor($date->getOffset()/3600);
  $minute = ($date->getOffset()%3600)/3600*60;

  $str = "";
  if($hour < 0) {
    $str = "-";
    $hour = -$hour;
  }
  else {
    $str = "+";
  }

  if($hour < 10) $str .= "0".$hour;
  else $str .= $hour;

  $str .= ":";

  if($minute < 10) $str .= "0".$minute;
  else $str .= $minute;

  return $str;
}

function twforms_get_bnode() {
  static $counter=0;
  return "_:a".$counter++;
}

function twforms_value_of_file_field($field) {
  if($_POST[$field."_method"]=="internal") {
    return $_POST[$field."_internal"];
  }
  else {
    return $_POST[$field."_external"];
  }
}

function twforms_field_is_required($field) {
  return $_POST[$field."_required"]=="true" ? TRUE : FALSE;
}

function twforms_value_of_alias_field($field) {
  if(0===strpos($_POST[$field."_base"],"http")) {
    return $_POST[$field."_base"]."/".$_POST[$field];
  }
  else
    return ($_SERVER["HTTPS"] ? "https://" : "http://").
      $_SERVER["SERVER_NAME"].base_path().
      $_POST[$field."_base"]."/".$_POST[$field];
}

function twforms_value_of_field_rel($field) {
  if(isset($_POST[$field."_rel"])) {
    return $_POST[$field."_rel"];
  }
  return null;
}

function twforms_value_of_field_rev($field) {
  if(isset($_POST[$field."_rev"])) {
    return $_POST[$field."_rev"];
  }
  return null;
}

function twforms_set_property(&$data, $s, $p, $o, $t, $d=null) {
  if($o=="") return;
  if(is_array($s)) {
    foreach($s as $s2) {
      if(!isset($data[$s2]))
	$data[$s2] = array();
      if(!isset($data[$s2][$p]))
	$data[$s2][$p] = array();
      if(is_array($o)) {
	foreach($o as $o2) {
	  $arr = array("type"=>$t,
		       "value"=>$o2);
	  if($d) {
	    $arr["datatype"] = $d;
	  }
	  $data[$s][$p][] = $arr;
	}
      }
      else {
	$arr = array("type"=>$t,
		     "value"=>$o);
	if($d) {
	  $arr["datatype"] = $d;
	}
	$data[$s][$p][] = $arr;
      }
    }
  }
  else {
    if(!isset($data[$s]))
      $data[$s] = array();
    if(!isset($data[$s][$p]))
      $data[$s][$p] = array();
    if(is_array($o)) {
      foreach($o as $o2) {
	$arr = array("type"=>$t,
		     "value"=>$o2);
	if($d) {
	  $arr["datatype"] = $d;
	}
	$data[$s][$p][] = $arr;
      }
    }
    else {
      $arr = array("type"=>$t,
		   "value"=>$o);
      if($d) {
	$arr["datatype"] = $d;
      }
      $data[$s][$p][] = $arr;
    }
  }
}

function twforms_process_field_results(&$data, $field, $uri, $obj=null) {
  $rel = $rev = $type = $dtype = null;
  $type = $_POST[$field."_type"];
  switch($type) {
  case "alias":
    $page = twforms_value_of_alias_field($field);
    if(!$obj)
      $obj = $page;
    /*
    if(twforms_field_is_required($field) && $obj=="") {
      return FALSE;
    }
    */
    $rel = twforms_value_of_field_rel($field);
    $rev = twforms_value_of_field_rev($field);
    $type = "uri";
    break;
  case "text":
    if(!$obj)
      $obj = $_POST[$field];
    $rel = twforms_value_of_field_rel($field);
    $rev = twforms_value_of_field_rev($field);
    $type = "literal";
    $dtype = "http://www.w3.org/2001/XMLSchema#string";
    break;
  case "daterange":
    $start = $_POST[$field."_start"];
    $end = $_POST[$field."_end"];
    $rel = twforms_value_of_field_rel($field);
    $rev = twforms_value_of_field_rev($field);
    if($start) {
      if(FALSE===strpos($start,"T"))
	$start .= "T00:00:00".twforms_get_timezone($start);
      $obj = twforms_get_bnode();
      $type = "bnode";
      twforms_set_property($data, $obj,
			   "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
			   "http://www.w3.org/2006/time#Interval", "uri");
      $startNode = twforms_get_bnode();
      twforms_set_property($data, $obj,
			   "http://www.w3.org/2006/time#hasBeginning",
			   $startNode, "bnode");
      twforms_set_property($data, $startNode,
			   "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
			   "http://www.w3.org/2006/time#Instant", "uri");
      twforms_set_property($data, $startNode,
			   "http://www.w3.org/2006/time#inXSDDateTime",
			   $start, "literal",
			   "http://www.w3.org/2001/XMLSchema#dateTime");
      if($end) {
	if(FALSE==strpos($end,"T"))
	  $end .= "T23:59:59".twforms_get_timezone($start);
	$endNode = twforms_get_bnode();
	twforms_set_property($data, $obj,
			     "http://www.w3.org/2006/time#hasEnd",
			     $endNode, "bnode");
	twforms_set_property($data, $endNode,
			     "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
			     "http://www.w3.org/2006/time#Instant", "uri");
	twforms_set_property($data, $endNode,
			     "http://www.w3.org/2006/time#inXSDDateTime",
			     $end, "literal",
			     "http://www.w3.org/2001/XMLSchema#dateTime");
      }
    }
    else {
      return $data;
    }
    break;
  case "url":
    if(!$obj)
      $obj = $_POST[$field];
    $rel = twforms_value_of_field_rel($field);
    $rev = twforms_value_of_field_rev($field);
    $type = "uri";
    break;
  case "email":
    if(!$obj)
      $obj = $_POST[$field];
    $obj = "mailto:".$obj;
    $rel = twforms_value_of_field_rel($field);
    $rev = twforms_value_of_field_rev($field);
    $type = "uri";
    break;
  case "textarea":
    if(!$obj)
      $obj = $_POST[$field];
    $obj = "<div xmlns=\"http://www.w3.org/1999/xhtml\">".$obj."</div>";
    $rel = twforms_value_of_field_rel($field);
    $rev = twforms_value_of_field_rev($field);
    $type = "literal";
    $dtype = "http://www.w3.org/1999/02/22-rdf-syntax-ns#XMLLiteral";
    break;
  case "instance":
    if(!$obj)
      $obj = $_POST[$field];
    if($obj == "") return $data;
    $rel = twforms_value_of_field_rel($field);
    $rev = twforms_value_of_field_rev($field);
    $type = "uri";
    break;
  case "subclass":
    if(!$obj)
      $obj = $_POST[$field];
    $rel = "http://www.w3.org/1999/02/22-rdf-syntax-ns#type";
    $type = "uri";
    break;
  case "file":
    if(!$obj)
      $obj = twforms_value_of_file_field($field);
    $rel = twforms_value_of_field_rel($field);
    $rev = twforms_value_of_field_rev($field);
    $type = "uri";
    break;
  case "date":
    if($obj)
      $val = $obj;
    else
      $val = $_POST[$field];
    if($val=="") return $data;
    if(FALSE===strpos($field,"T"))
      $val = $val."T00:00:00".twforms_get_timezone($val);
    $rel = twforms_value_of_field_rel($field);
    $bnode = twforms_get_bnode();
    twforms_set_property($data, $bnode,
			 "http://www.w3.org/2006/time#inXSDDateTime",
			 $val, "literal",
			 "http://www.w3.org/2001/XMLSchema#dateTime");
    twforms_set_property($data, $bnode,
			 "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
			 "http://www.w3.org/2006/time#Instant", "uri");
    $obj = $bnode;
    $type = "bnode";
    break;
  default:
    drupal_set_message("Field '$field' of unknown type '$type'.","warning");
    return false;
  }
  // disallow control characters except \x0A and \x0D (cr and lf)
  $obj = preg_replace('/[\x00-\x09\x0B\x0C\x0E-\x1F]/', '', $obj);
  if($rel) {
    if($obj == "") return $data;
    twforms_set_property($data, $uri, $rel, $obj, $type, $dtype);
  }
  else if($rev) {
    if($obj == "") return $data;
    if($uri == "") return $data;
    twforms_set_property($data, $obj, $rev, $uri, (0===strpos($uri,"_:") ? "bnode" : "uri"));
  }
  else {
    drupal_set_message("No rel or rev set for input '$field'.","error");
    return false;
  }
  return $data;
}

function twforms_process_compound_results(&$data, $field, $uri) {
  $subfields = array();
  $class = $_POST[$field."_class"];
  foreach($_POST as $subfield => $value) {
    if(0===strpos($subfield,$field."_")) {
      $pos = strpos($subfield,"_type",strlen($field)+1);
      if($pos!==FALSE) {
	$substr = substr($subfield,0,$pos);
	$subfields[$substr] = true;
      }
    }
  }

  $values = array();
  $hasvalue = false ;
  foreach($subfields as $subfield => $exists) {
    $info = array();
    $info["type"] = $_POST[$subfield."_type"];
    $myvalue = $_POST[$subfield];

    // let's see if this compound has any values. If it doesn't, then we don't want to add
    // any data.
    if( is_array( $myvalue ) )
    {
	// we want to ignore the subclass, since it will always have a value of at least
	// the top most class in the class tree.
	if( $info["type"] != "subclass" )
	{
	    // implode will create a single string with all of the values. If that string
	    // is empty, then there are no values
	    $fullvalue = implode( "", $myvalue ) ;
	    if( strlen( $fullvalue ) > 0 )
	    {
		// hey ... we have values
		$hasvalue = true ;
	    }
	}
    }
    else
    {
	// simple test on a string. If the string has length, then there is a value
	// but, you ask, what if they just hit space a bunch of times.
	$myvalue = trim( $myvalue ) ;
	if( strlen( $myvalue ) > 0 )
	{
	    $hasvalue = true ;
	}
    }
    $info["value"] = $myvalue;
    $values[$subfield] = $info;
  }

  // we don't want to error out if there aren't any values, but we also don't want to add values
  if( $hasvalue == false )
  {
    drupal_set_message("twforms_process_compound_results field '$field' has no values","status");
    return $data ;
  }

  if($_POST[$field."_many"]=="true") {
    $order = $_POST[$field."_order"];
    if($order == "false") $order = null;
    $len = 0;
    foreach($values as $subfield => $bits) {
      $len = count($bits["value"]);
      break;
    }
    for($i=0;$i<$len;$i++) {
      $self = twforms_get_bnode();
      twforms_set_property($data, $self,
                           "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
                           $class, "uri");
      foreach($values as $subfield => $bits) {
	$value = $bits["value"][$i];
	if(!twforms_process_field_results($data, $subfield, $self, $value)) {
	  drupal_set_message("twforms_process_field_results returned false for subfield '$subfield' of compound element '$field'.","error");
	  return false;
	}
      }
      if($order)
	twforms_set_property($data, $self, $order, $i+1, "literal",
			     "http://www.w3.org/2001/XMLSchema#int");
      $rel = twforms_value_of_field_rel($field);
      $rev = twforms_value_of_field_rev($field);
      if($rel) {
	twforms_set_property($data, $uri, $rel, $self, "bnode");
      }
      else if($rev) {
	twforms_set_property($data, $self, $rev, $uri, "uri");
      }
      else {
	drupal_set_message("Compound form '$field' not related to parent object by rel nor rev attribute.","error");
	return false;
      }
    }
  }
  else {
    $self = twforms_get_bnode();
    foreach($values as $subfield => $bits) {
      if(!twforms_process_field_results($data, $subfield, $self))
	return false;
    }
    $rel = twforms_value_of_field_rel($field);
    $rev = twforms_value_of_field_rev($field);
    if($rel) {
      twforms_set_property($data, $uri, $rel, $self, "bnode");
    }
    else if($rev) {
      twforms_set_property($data, $self, $rev, $uri, (0===strpos($uri,"_:") ? "bnode" : "uri"));
    }
    else {
      drupal_set_message("Compound form '$field' not related to parent object by rel nor rev attribute.","error");
      return false;
    }
  }
  return $data;
}

function twforms_process_POST() {
  global $twforms_uri;
  $data = array();

  $fields = array();
  foreach($_POST as $field => $value) {
    if(FALSE!==($pos=strrpos($field,"_type"))) {
      $name = substr($field,0,$pos);
      if(FALSE===strpos($name,"_")) {
	$fields[$name] = $_POST[$name."_type"];
      }
    }
  }
  /*
  if(!isset($fields["uri"])) {
    drupal_set_message("No URI specified in form.","error");
    return null;
  }
  */
  if(isset($fields["uri"])) {
    $uri = ($fields["uri"] == "file" ? twforms_value_of_file_field("uri") :
	    ($fields["uri"] == "alias" ? twforms_value_of_alias_field("uri") :
	     $_POST["uri"]));
  }
  else {
    $uri = twforms_segment_uri($_POST["class"]);
    $uri = $uri['id'];
    $curTime = gettimeofday();
    $uri .= "_".$curTime["sec"]."_".$curTime["usec"];
    $uri = TWSparql::getEngine()->getIbase().$uri;
  }
  if($uri=="") {
    drupal_set_message("No URI specified in form.","error");
    return null;
  }
  $twforms_uri = $uri;
  twforms_set_property($data, $uri,
		       "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
		       $_POST["class"], "uri");
  foreach($fields as $field => $type) {
    if($field=="uri") continue;
    $type = $_POST[$field."_type"];
    switch($type) {
    case "compound":
      $obj = twforms_process_compound_results($data, $field, $uri);
      if(!$obj) return false;
      break;
    default:
      $obj = twforms_process_field_results($data, $field, $uri);
      if(!$obj) return false;
      break;
    }
  }

  return $data;
}

/**
 * Function to convert PHP associative array to RDF
 */
function twforms_serialize_rdf($format, $arr) {
  if ($format == "ttl") {
    $output = twforms_serialize_turtle($arr);
  }
  else if($format == "xml") {
    $output = twforms_serialize_xml($arr);
  }
  return $output;
}

function twforms_segment_uri($uri) {
  $i = strlen($uri)-1;
  // Taken from http://www.w3.org/TR/REC-xml-names/#NT-NCName
  while($i>0) {
    if(("A"<=$uri[$i]&&"Z">=$uri[$i])||
       $uri[$i]=="_"||
       ("a"<=$uri[$i]&&"z">=$uri[$i])||
       ("\xC0"<=$uri[$i]&&"\xD6">=$uri[$i])||
       ("\xD8"<=$uri[$i]&&"\xF6">=$uri[$i])||
       ("\xF8"<=$uri[$i]&&"\xFF">=$uri[$i])||
       $uri[$i]=="-"||
       $uri[$i]=="."||
       ("0"<=$uri[$i]&&"9">=$uri[$i])||
       $uri[$i]=="\xB7") {
      $i--;
      continue;
    }
    $i++;
    return array("ns"=>substr($uri,0,$i),"id"=>substr($uri,$i));
  }
  return array("ns"=>"","id"=>$uri);
}

function twforms_serialize_xml($arr) {
  global $twforms_namespaces;
  $ns_lookup = array();
  foreach($twforms_namespaces as $ns => $uri) {
    $ns_lookup[$uri] = $ns;
  }
  $counter = 0;
  $result = "";
  foreach($arr as $subj => $props) {
    $sSplit = twforms_segment_uri($subj);
    if($sSplit["ns"]!="_:"&&!isset($ns_lookup[$sSplit["ns"]])) {
      $ns = "j.$counter";
      $counter++;
      $twforms_namespaces[$ns] = $sSplit["ns"];
      $ns_lookup[$sSplit["ns"]] = $ns;
    }
    $result .= "<rdf:Description ";
    if($sSplit["ns"]=="_:") {
      $result .= "rdf:nodeID=\"".$sSplit["id"]."\">\r\n";
    }
    else {
      $result .= "rdf:about=\"".$subj."\">\r\n";
    }
    foreach($props as $prop => $objs) {
      $pSplit = twforms_segment_uri($prop);
      if(!isset($ns_lookup[$pSplit["ns"]])) {
	$ns = "j.$counter";
	$counter++;
	$twforms_namespaces[$ns] = $pSplit["ns"];
	$ns_lookup[$pSplit["ns"]] = $ns;
      }
      foreach($objs as $obj) {
	if($obj["type"]=="uri") {
	  $result .= "<".$ns_lookup[$pSplit["ns"]].":".$pSplit["id"].
	    " rdf:resource=\"".$obj["value"]."\"/>\r\n";
	}
	else if($obj["type"]=="literal") {
	  $result .= "<".$ns_lookup[$pSplit["ns"]].":".$pSplit["id"];
	  if(isset($obj["datatype"]) && $obj["datatype"]=="http://www.w3.org/1999/02/22-rdf-syntax-ns#XMLLiteral") {
	    $result .= " rdf:parseType=\"Literal\"";
          }
	  else if(isset($obj["datatype"])) {
	    $result .= " rdf:datatype=\"".$obj["datatype"]."\"";
	  }
	  else if(isset($obj["lang"])) {
	    $result .= " xml:lang=\"".$obj["lang"]."\"";
	  }
	  $result .= ">";
	  $result .= $obj["value"];
	  $result .= "</".$ns_lookup[$pSplit["ns"]].":".$pSplit["id"].">\r\n";
	}
	else if($obj["type"]=="bnode") {
	  $result .= "<".$ns_lookup[$pSplit["ns"]].":".$pSplit["id"].
	    " rdf:nodeID=\"".substr($obj["value"],2)."\"/>\r\n";
	}
      }
    }
    $result .= "</rdf:Description>\r\n";
  }
  $header = "<?xml version=\"1.0\"?><rdf:RDF";
  foreach($ns_lookup as $uri => $ns) {
    if($uri!="")
      $header .= " xmlns:$ns=\"$uri\"";
  }
  $header .= ">\r\n";
  $result = $header . $result;
  $result .= "</rdf:RDF>";
  return $result;
}

function twforms_serialize_turtle($arr) {
  $output = "";
  foreach($arr as $s => $pmap) {
    foreach($pmap as $p => $oarr) {
      foreach($oarr as $o) {
        $sb = preg_match("/^_:/",$s); //check if subject is blank
	if ($sb) $output .= "$s <$p> ";
	else $output .= "<$s> <$p> ";
	if ($o['type'] == 'uri') //print URI object
	  $output .= '<'.$o['value']."> .\n";
	else if ($o['type'] == 'literal') { //print literal object
	  $val = $o['value'];
	  $val = preg_replace('/\r/','\\r',$val);
	  $val = preg_replace('/\n/','\\n',$val);
	  $output .= '"'.preg_replace("/\"/","\\\"",$val).'"';
	  if (array_key_exists('datatype',$o)) $output .= '^^<'.$o['datatype'].'>';
	  if (array_key_exists('lang',$o)) $output .= '@'.$o['lang'];
	  $output .= " .\n";
	} else if ($o['type'] == 'bnode') { //print blank object
	  $output .= $o['value']." .\n";
	}
      }
    }
  }
  return $output;
}

function twforms_process_sparql_select($content) {
  $xml = simplexml_load_string($content);
  $results = array();
  foreach ($xml->results->result as $result) {
    $arr = array();
    foreach($result->binding as $binding) {
      $name = $binding['name'];
      $arr["$name"] = (string) $binding->children();
    }
    array_push($results,$arr);
  }
  return $results;
}

function twforms_create_page($content) {
  global $user,$twforms_uri;
  module_load_include("inc","node","node.pages");
  if(isset($twforms_uri))
    $uri = $twforms_uri;
  else if ($_POST['uri_type'] == 'file')
    $uri = twforms_value_of_file_field('uri');
  else if ($_POST['uri_type'] == 'alias')
    $uri = twforms_value_of_alias_field('uri');
  else $uri = $_POST['uri'];
  if (isset($_POST["contenttype"]))
    $type = $_POST["contenttype"];
  else
    $type = "page";
  $content = preg_replace('/\\$uri/', $uri, $content);
  $node = array("type" => $type);
  drupal_set_message("twforms_create_page: type = '$type'","status");
  $form_state = array();
  $form_state['values'] = array();
  $mytitle = $_POST['title'];
  $form_state['values']['title'] = $mytitle;
  drupal_set_message("twforms_create_page: title = '$mytitle'","status");
  $form_state['values']['body'] = $content;
  drupal_set_message("twforms_create_page: content = '$content'","status");
  $form_state['values']['uid'] = $user->uid;
  drupal_set_message("twforms_create_page: user id = '$user->uid'","status");
  $form_state['values']['name'] = $user->name;
  drupal_set_message("twforms_create_page: user name = '$user->name'","status");
  $form_state['values']['status'] = 1;

  // if an alias is specified, then we set the URL path settings to that
  if (isset($_POST['alias']))
  {
    $mypath = $_POST["alias_base"] . '/' . $_POST["alias"];
    $form_state['values']['path'] = $mypath ;
    drupal_set_message("twforms_create_page: path = '$mypath'","status");
  }
  else
  {
    drupal_set_message("twforms_create_page: path = NONE SET","status");
  }
  $form_state['values']['op'] = t("Save");
  drupal_set_message("drupal_execute($type.\"_node_form\", $form_state, (object)$node);","status" ) ;
  drupal_execute($type."_node_form", $form_state, (object)$node);

  // redirect to the node/number page, not the local alias specified
  $myredirect = "Location: ".base_path()."node/".$form_state["nid"];
  drupal_set_message("twforms_create_page: redirect = '$myredirect'","status");
  header($myredirect);
  //die;
}

function twforms_get_endpoint() {
  return TWSparql::getEngine()->getEndpoint();
}
