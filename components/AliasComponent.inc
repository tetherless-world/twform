<?php

class AliasComponent extends SimpleComponent {
  protected $base;
  protected $value;
  protected $allow_absolute = FALSE;

  public function __construct() {
    $this->type = "alias";
  }
  
  public function getValue() {
    return $this->value;
  }
  
  public function toSPARUL($node) {
    if(!$this->value) return "";
    if($this->rel) {
      return "$node <$this->rel> <$this->value> . ";
    } else if($this->rev) {
      return "<$this->value> <$this->rev> $node . ";
    } else {
      user_error("Neither rel nor rev specified on URL component $this->id", E_USER_WARNING);
      return "";
    }
  }

  public function prepopulate($inst, $top) {
    if($top) {
      $engine = TWSparql::getEngine();
      $endpoint = $engine->getEndpoint();
      $err = NULL;
      if($this->rel) {
        $query = "SELECT ?x WHERE { <$inst> <$this->rel> ?x }";
      } else if($this->rev) {
        $query = "SELECT ?x WHERE { ?x <$this->rev> <$inst> }";
      } else {
        return NULL;
      }
      $entries = $engine->executeQueryJSON($endpoint, $query, $err);
      if(is_array($entries) && count($entries) > 0) {
        $this->value = $entries[0]["x"];
      }
      return $this->value;
    } else {
      if($this->rel) {
        return array(array($inst, $this->rel, $this->getVar()));
      } else if($this->rev) {
        return array(array($this->getVar(), $this->rev, $inst));
      } else {
        return NULL;
      }
    }
  }

  public function postpopulate($inst, $entries) {
    $var = substr($this->getVar(),1);
    $this->value = $entries[0][$var];
  }

  public function processParams(&$params) {
    $success = parent::processParams($params);
    if(!isset($params["base"])) {
      $this->getLogger()->error("Form input '$this->id' does not have required parameter 'base'.");
      return false;
    }
    if(isset($params["external"])) {
      if(strtoupper($params["external"])=="TRUE") {
        $this->allow_absolute = TRUE;
      }
    }
    $this->base = $params["base"];
    return $success;
  }

  public function render() {
    global $base_url;
    $res = "";
    $part = NULL;
    if(0===strpos($this->base,"http://")) {
      $res .= $this->base."/";
      if($this->value != NULL) {
        if(0===strpos($this->value,$this->base)) {
          $part = substr($this->value, strlen($this->base)+1);
        }
        else {
          $part = $this->value;
        }
      }
    } else {
      $altbase = $this->getForm()->getAltBase();
      if($altbase != "")
        $base = $altbase . (strrpos($altbase, "/")==(strlen($altbase)-1) ? '' : '/') .
          $this->base . '/';
      else if(isset($base_url) && $base_url != "")
        $base = $base_url . "/" . $this->base . "/";
      else
        $base = (isset($_SERVER["HTTPS"]) && $_SERVER["HTTPS"] ?
          "https://" : "http://").$_SERVER["SERVER_NAME"].
                base_path().$this->base."/";
      $res .= " ".$base;
      if($this->value != NULL) {
        if(0===strpos($this->value,$base)) {
          $part = substr($this->value, strlen($base));
        } else {
          $part = $this->value;
        }
      }
    }
    $res .= '<input name="'.$this->id.'" type="text" '.
        ($part != NULL ? 'value="'.$part.'" ' : '').'/>';
    $res .= '<input name="'.$this->id.'_base" type="hidden" value="'.$this->base.'"/>';
    return $res;
  }

  public function init($space) {
    return "";
  }

  public function processPOST(&$data, $field, $uri, $obj=NULL) {
    $page = TWFormUtil::valueOfAliasField($field, $this->form->getBasePath());
    if(!$obj) {
      $obj = $page;
    }
    $this->rel = TWFormUtil::valueOfFieldRel($field);
    $this->rev = TWFormUtil::valueOfFieldRev($field);
    $type = "uri";
    $dtype = null;
    return parent::postProcessPOST($data, $uri, $obj, $type, $dtype);
  }

  public function dump($spaces)
  {
      $this->getLogger()->logStatus( "$spaces AliasComponent:" ) ;
      $newspaces = $spaces . "&nbsp;&nbsp;&nbsp;&nbsp;" ;
      parent::dump( $newspaces ) ;

      $this->getLogger()->logStatus( "$newspaces base = $this->base" ) ;
  }
};
