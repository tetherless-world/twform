<?php

class InstanceComponent extends SimpleComponent {
  protected $class;
  protected $multiple=false;
  protected $size=null;
  protected $values = NULL;

  public function __construct() {
    $this->type = "instance";
  }
  
  public function getValue() {
    return $this->values;
  }
  
  public function toSPARUL($node) {
    if(!$this->rel && !$this->rev) {
      user_error("Neither rel nor rev specified on instance component $this->id", E_USER_WARNING);
      return "";
    }
    if($this->rel) {
      $str = "$node <$this->rel> ";
      $first = TRUE;
      foreach($this->values as $i => $uri) {
        if($first) {
          $str .= "<$uri>";
          $first = FALSE;
        } else {
          $str .= ", <$uri>";
        }
      }
      return $str." . ";
    } else {
      $str = "";
      foreach($this->values as $i => $uri) {
        $str .= "<$uri> <$this->rev> $i . ";
      }
      return $str;
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
        $this->values = array();
        foreach($entries as $entry) {
          $this->values[] = $entry["x"];
        }
      }
      return $this->values;
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
    $this->values = array();
    $var = substr($this->getVar(),1);
    foreach($entries as $entry) {
      if(isset($entry[$var]))
        $this->values[] = $entry[$var];
    }
  }

  public function processParams(&$params) {
    $success = parent::processParams($params);
    if(!isset($params["class"])) {
      drupal_set_message("Form input '$this->id' does not have required parameter 'class'.","error");
      return false;
    }
    $this->class = $this->resolve($params["class"]);
    if(isset($params["multiple"])&&$params["multiple"]=="multiple") {
      $this->multiple = true;
    }
    if(isset($params["size"]))
      $this->size = $params["size"];
    return $success;
  }

  public function render() {
    $res = '<select name="'.$this->id.($this->multiple || $this->isArray ? "[]" : "").'"';
    if($this->multiple)
      $res .= ' multiple="multiple"';
    if($this->size)
      $res .= ' size="'.$this->size.'"';
    $res .= ">";
    if(!$this->multiple)
      $res .= "<option value=\"\">None</option>";
    $bits = $this->individualsForClass($this->class);
    $first = TRUE;
    foreach($bits as $uri=>$label) {
      $res .= "<option value=\"$uri\" ";
      if($first && $this->values != NULL && in_array($uri,$this->values)) {
        $res .= 'selected="selected"';
        if(!$this->multiple) $first = false;
      }
      $res .= ">$label</option>";
    }
    $res .= "</select>";
    return $res;
  }

  public function init($space) {
    /*
    return 
      $space.'var self = $(\'select[name="'.$this->id.($this->multiple || $this->isArray ? "[]" : "").'"]\',this);'.EOL.
      $space.'TWForms.classBasedSelect(self,"'.$this->class.'");';
    */
    return "";
  }

  public function dump($spaces)
  {
      $this->getLogger()->logStatus( "$spaces InstanceComponent:" ) ;
      $newspaces = $spaces . "&nbsp;&nbsp;&nbsp;&nbsp;" ;
      parent::dump( $newspaces ) ;

      $this->getLogger()->logStatus( "$newspaces class = $this->class" ) ;

      if( $this->multiple ) $this->getLogger()->logStatus( "$newspaces multiple = true" ) ;
      else $this->getLogger()->logStatus( "$newspaces multiple = false" ) ;

      if( $this->size != null ) $this->getLogger()->logStatus( "$newspaces size = $this->size" ) ;
  }
};
