<?php

class InstanceComponent extends SimpleComponent {
  protected $class;
  protected $multiple=false;
  protected $size=null;
  protected $placeholder="";
  protected $values = NULL;
  protected $newObj = NULL;

  public function __construct() {
    $this->type = "instance";
  }
  
  public function getValue() {
    return $this->values;
  }
  
  public function toSPARUL() {
    $output = parent::toSPARUL();
    if($this->newObj) $output.=$this->newObj->toSPARUL();
    return $output;
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
      $this->logger->logError("Form input '$this->id' does not have required parameter 'class'.");
      return false;
    }
    $this->class = $this->resolve($params["class"]);
    if(isset($params["multiple"])&&$params["multiple"]=="multiple") {
      $this->multiple = true;
    }
    if(isset($params["size"]))
      $this->size = $params["size"];
    if(isset($params["placeholder"]))
      $this->placeholder = $params["placeholder"];
    return $success;
  }

  public function render() {
    $res = '<select class="twforms-select-'.$this->id.'" style="width:320px;" name="'.$this->id.($this->multiple || $this->isArray ? "[]" : "").'"';
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
    $res .= '<input type="hidden" name="'.$this->id.'_class" value="'.$this->class.'"/>';
    return $res;
  }

  public function init($space) {
    return
      $space.'var self = $(\'select[name^="'.$this->id.'"]\',this);'.EOL.
      $space.'self.select2({placeholder:"'.$this->placeholder.'"});';
    /*
    return 
      $space.'var self = $(\'select[name="'.$this->id.($this->multiple || $this->isArray ? "[]" : "").'"]\',this);'.EOL.
      $space.'TWForms.classBasedSelect(self,"'.$this->class.'");';
    */
    return "";
  }

  public function processPOST(&$data, $field, $uri, $obj=NULL) {
    $dtype = null;
    if(!$obj) {
      $obj = $_POST[$field];
      if(strpos($obj, "http") != 0) {
        $this->class = $_POST[$field."_class"];
        if(!$this->class) {
          $this->logger->logError("Instance form field $field is missing class field.");
        } else {
          $this->newObj = NewInstance::getNewInstance($this->class, $obj);
          if(!$this->newObj) {
            $this->logger->logError("Instance form field '$field' class '$this->class' does not have a corresponding new object.");
          } else {
            $obj = $this->newObj->createNew();
          }
        }
      }
    }
    if($obj == "") return true;
    $this->rel = TWFormUtil::valueOfFieldRel($field);
    $this->rev = TWFormUtil::valueOfFieldRev($field);
    $type = "uri";
    return parent::postProcessPOST($data, $uri, $obj, $type, $dtype);
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
