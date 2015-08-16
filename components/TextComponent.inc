<?php

class TextComponent extends SimpleComponent {
  protected $contents;
  protected $value = NULL;

  public function __construct() {
    $this->type = "text";
  }

  public function validate() {
    if($this->required && $this->value=="") {
      return array("Field '".$this->id."' cannot be left blank.");
    }
    return array();
  }

  public function getValue() {
    return $this->value;
  }

  public function toSPARUL($node) {
    if($this->rel) {
      // TODO(ewpatton): escape $node """ becomes ""\"
      return "$node <$this->rel> \"\"\"$node\"\"\" . ";
    } else if($this->rev) {
      user_error("Rev should not be specified for a text component",
                 E_USER_WARNING);
      return "";
    } else {
      user_error("Neither rel nor rev specified for text component $this->id",
                 E_USER_WARNING);
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

  public function dump($spaces)
  {
      $this->getLogger()->logStatus( "$spaces TextComponent:" ) ;
      $newspaces = $spaces . "&nbsp;&nbsp;&nbsp;&nbsp;" ;
      parent::dump( $newspaces ) ;

      $this->getLogger()->logStatus( "$newspaces contents = $this->contents" ) ;
  }
};
