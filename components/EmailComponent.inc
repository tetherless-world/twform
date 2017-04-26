<?php

class EmailComponent extends SimpleComponent {
  protected $contents;
  protected $value = NULL;

  static public $regex = "[a-z0-9!#$%&'*+/=?^_`{|}~-]+?:\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*@?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+a-z0-9](?:[a-z0-9-]*[a-z0-9])?";
  
  public function __construct() {
    $this->type = "email";
  }
  
  public function getValue() {
    
  }
  
  public function toSPARUL($node) {
    
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
        $this->value = substr($entries[0]["x"],7);
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
    $this->value = substr($entries[0][$var],7);
  }

  public function processParams(&$params) {
    $success = parent::processParams($params);
    if(isset($params["contents"]))
      $this->contents = $params["contents"];
    return $success;
  }

  public function render() {
    $res = '<input type="text" name="'.$this->id.'" '.
      ($this->value != NULL ? 'value="'.$this->value.'" ' : "" ).'/>';
    return $res;
  }

  public function init($space) {
    return "";
  }

  public function dump($spaces)
  {
      $this->getLogger()->logStatus( "$spaces EmailComponent:" ) ;
      $newspaces = $spaces . "&nbsp;&nbsp;&nbsp;&nbsp;" ;
      parent::dump( $newspaces ) ;

      $this->getLogger()->logStatus( "$newspaces contents = $this->contents" ) ;
  }
};
