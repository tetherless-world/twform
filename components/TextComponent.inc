<?php

class TextComponent extends SimpleComponent {
  protected $contents;
  protected $value = NULL;

  public function __construct() {
    $this->type = "text";
  }

  public function getValue() {
    return $this->value;
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
    $var = substr($this->getVar(),1);
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
      ($this->autoalias != FALSE ? ' autoalias="true" ' : "").
      ($this->value != NULL ? ' value="'.$this->value.'" ' : "").'/>';
    return $res;
  }

  public function init($space) {
    return "";
  }

  public function processPOST(&$data, $field, $uri, $obj=NULL) {
    if(!$obj) {
      $obj = $_POST[$field];
    }
    $this->rel = TWFormUtil::valueOfFieldRel($field);
    $this->rev = TWFormUtil::valueOfFieldRev($field);
    $type = "literal";
    $dtype = "http://www.w3.org/2001/XMLSchema#string";
    return parent::postProcessPOST($data, $uri, $obj, $type, $dtype);
  }

  public function dump($spaces)
  {
      $this->getLogger()->logStatus( "$spaces TextComponent:" ) ;
      $newspaces = $spaces . "&nbsp;&nbsp;&nbsp;&nbsp;" ;
      parent::dump( $newspaces ) ;

      $this->getLogger()->logStatus( "$newspaces contents = $this->contents" ) ;
  }
};
