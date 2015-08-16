<?php

class UrlComponent extends SimpleComponent {
  protected $contents;
  protected $value = NULL;
  
  static public $regex = "^[a-z][a-z0-9+\\.\\-]*:";

  public function __construct() {
    $this->type = "url";
  }
  
  public function validate() {
    if($this->required) {
      if($this->value == "") {
        return array("Field '".$this->id."' cannot be left blank.");
      }
    }
    if($this->value=="") return array();
    if(0==preg_match($regex, $this->value)) {
      return array("The value '".$this->id."' does not have a valid URI scheme.");
    } else {
      $parts = explode(":", $this->value);
      if($parts[0]=="http" || $parts[0]=="https") {
        // TODO: more validation on HTTP URLs
        return array();
      }
    }
    return array();
  }
  
  public function getValue() {
    return $this->value;
  }
  
  public function toSPARUL($node) {
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
      $entries = $engine->executeQueryJSON( $endpoint, $query, $err );
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
    $var = substr($this->getVar());
    $this->value = $entries[0][$var];
  }

  public function processParams(&$params) {
    $success = parent::processParams($params);
    if(isset($params["contents"]))
      $this->contents = $params["contents"];
    return $success;
  }

  public function render() {
    $res = '<input type="text" name="'.$this->id.'" '.
      ($this->value != NULL ? ' value="'.$this->value.'" ' : "").'/>';
    return $res;
  }

  public function init($space) {
    return "";
  }

  public function dump($spaces)
  {
      $this->getLogger()->logStatus( "$spaces UrlComponent:" ) ;
      $newspaces = $spaces . "&nbsp;&nbsp;&nbsp;&nbsp;" ;
      parent::dump( $newspaces ) ;

      $this->getLogger()->logStatus( "$newspaces contents = $this->contents" ) ;
  }
};
