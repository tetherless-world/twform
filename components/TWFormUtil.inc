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

};

?>
