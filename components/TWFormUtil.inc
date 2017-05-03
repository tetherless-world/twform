<?php

class TWFormUtil {

  /**
   * Generates a fresh bnode variable for use in queries
   * @staticvar int $counter Keeps track of the next available bnode
   * @return string Blank node identifier
   */
  public static function getBnode() {
    static $counter=0;
    return "_:a".$counter++;
  }

  /**
   * Returns the value of a file field based on whether a URI was specified or a document was uploaded
   * @param string $field Name of the field to check
   * @return string Value of the file field, null if field is invalid.
   */
  public function valueOfFileField($field) {
    if(!isset($_POST[$field."_method"])) {
      return null;
    }
    else if($_POST[$field."_method"]=="internal") {
      return $_POST[$field."_internal"];
    }
    else {
      return $_POST[$field."_external"];
    }
  }

  /**
   * Returns the absolute path for an alias field
   * @param string $field Name of field to retrieve
   * @return string Absolute path identified by the field
   */
  public static function valueOfAliasField($field, $basePath) {
    if(0===strpos($_POST[$field."_base"],"http")) {
      return $_POST[$field."_base"]."/".$_POST[$field];
    }
    else {
      return ($_SERVER["HTTPS"] ? "https://" : "http://").
        $_SERVER["SERVER_NAME"].$basePath.
        $_POST[$field."_base"]."/".$_POST[$field];
    }
  }

  public static function valueOfFieldRel($field) {
    if(isset($_POST[$field."_rel"])) {
      return $_POST[$field."_rel"];
    }
    return null;
  }

  public static function valueOfFieldRev($field) {
    if(isset($_POST[$field."_rev"])) {
      return $_POST[$field."_rev"];
    }
    return null;
  }

  public static function setProperty(&$data, $s, $p, $o, $t, $d=null) {
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

  public static function createComponent($type, $form, $logger) {
    $obj = null;
    switch($type) {
    case "compound":
      $obj = new CompoundComponent();
      break;
    case "text":
      $obj = new TextComponent();
      break;
    case "url":
      $obj = new UrlComponent();
      break;
    case "email":
      $obj = new EmailComponent();
      break;
    case "file":
      $obj = new FileComponent();
      break;
    case "alias":
      $obj = new AliasComponent();
      break;
    case "instance":
      $obj = new InstanceComponent();
      break;
    case "subclass":
      $obj = new SubclassComponent();
      break;
    case "date":
      $obj = new DateComponent();
      break;
    case "daterange":
      $obj = new DateRangeComponent();
      break;
    case "textarea":
      $obj = new TextareaComponent();
      break;
    }

    if($obj) {
      $obj->setForm($form);
      $obj->setLogger( $logger ) ;
    }

    return $obj;
  }

  /**
   * Converts the form POST data into a SPARQL 1.1 Update request
   * @return string SPARUL request for updating the triple store
   */
  public static function toSPARUL($data) {
    $output = "";
    foreach($data as $s => $pmap) {
      foreach($pmap as $p => $oarr) {
        foreach($oarr as $o) {
          $sb = preg_match("/^_:/",$s); //check if subject is blank
          if ($sb) $output .= "$s <$p> ";
          else $output .= "<$s> <$p> ";
          if ($o['type'] == 'uri') //print URI object
            $output .= '<'.$o['value']."> .\n";
          else if ($o['type'] == 'literal') { //print literal object
            $val = $o['value'];
            $val = preg_replace('/\r/','',$val);
            $val = preg_replace('/\n/','',$val);
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

  public static function toRDF($data) {
    $nsLookup = array();
    foreach(TWForm::$namespaces as $ns => $uri) {
      $nsLookup[$uri] = $ns;
    }
    $counter = 0;
    $result = "";
    foreach($data as $subj => $props) {
      $sSplit = TWForm::segmentURI($subj);
      if($sSplit["ns"]!="_:"&&!isset($nsLookup[$sSplit["ns"]])) {
        $ns = "j.$counter";
        $counter++;
        TWForm::$namespaces[$ns] = $sSplit["ns"];
        $nsLookup[$sSplit["ns"]] = $ns;
      }
      $result .= "<rdf:Description ";
      if($sSplit["ns"]=="_:") {
        $result .= "rdf:nodeID=\"".$sSplit["id"]."\">\r\n";
      } else {
        $result .= "rdf:about=\"".$subj."\">\r\n";
      }
      foreach($props as $prop => $objs) {
        $pSplit = TWForm::segmentURI($prop);
        if(!isset($nsLookup[$pSplit["ns"]])) {
          $ns = "j.$counter";
          $counter++;
          TWForm::$namespaces[$ns] = $pSplit["ns"];
          $nsLookup[$pSplit["ns"]] = $ns;
        }
        foreach($objs as $obj) {
          if($obj["type"]=="uri") {
            $result .= "<".$nsLookup[$pSplit["ns"]].":".$pSplit["id"].
              " rdf:resource=\"".$obj["value"]."\"/>\r\n";
          } else if($obj["type"]=="literal") {
            $result .= "<".$nsLookup[$pSplit["ns"]].":".$pSplit["id"];
            if(isset($obj["datatype"]) && $obj["datatype"] ==
               "http://www.w3.org/1999/02/22-rdf-syntax-ns#XMLLiteral") {
              $result .= " rdf:parseType=\"Literal\"";
            } else if(isset($obj["datatype"])) {
              $result .= " rdf:datatype=\"".$obj["datatype"]."\"";
            } else if(isset($obj["lang"])) {
              $result .= " xml:lang=\"".$obj["lang"]."\"";
            }
            $result .= ">";
            $result .= $obj["value"];
            $result .= "</".$nsLookup[$pSplit["ns"]].":".$pSplit["id"].">\r\n";
          } else if($obj["type"]=="bnode") {
            $result .= "<".$nsLookup[$pSplit["ns"]].":".$pSplit["id"].
              " rdf:nodeID=\"".substr($obj["value"],2)."\"/>\r\n";
          }
        }
      }
      $result .= "</rdf:Description>\r\n";
    }
    $header = "<?xml version=\"1.0\"?><rdf:RDF";
    foreach($nsLookup as $uri => $ns) {
      if($uri!="")
        $header .= " xmlns:$ns=\"$uri\"";
    }
    $header .= ">\r\n";
    $result = $header . $result;
    $result .= "</rdf:RDF>";
    return $result;
  }

  public static function createPath($basePath, $name) {
    $newPath = "";
    if($basePath!="") {
      if(!$name) {
        $name = ($_POST["uri_type"]=="file" ? TWFormUtil::valueOfFileField("uri") : $_POST["uri"]);
        $name = TWForm::segmentURI($_POST["class"]);
        $name = $uri['id'];
      }
      $curTime = gettimeofday();
      $uri = $name."_".$curTime["sec"]."_".$curTime["usec"];
      if(strrpos($path,"/")==strlen($path)-1)
        $path .= "/";
      $newPath = $path.$uri;
    }
    return $newPath;
  }
};

?>
