<?php

class SubclassComponent extends SimpleComponent {
  protected $multiple=false;
  protected $size=null;
  protected $values=NULL;

  public function __construct() {
    $this->type = "subclass";
  }
  
  public function getValue() {
    return $this->values;
  }
  
  public function toSPARUL($node) {
    if(!$this->rel && !$this->rev) {
      user_error("Neither rev nor rel specified for subclass component $this->id", E_USER_WARNING);
      return "";
    }
    if(!$this->rel) {
      user_error("Rev should not be specified for subclass component $this->id", E_USER_WARNING);
    }
    $str = "$node rdf:type ";
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
  }

  public function prepopulate($inst, $top) {
    if($top) {
      $engine = TWSparql::getEngine();
      $endpoint = $engine->getEndpoint();
      $query = "PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#> ".
        "SELECT ?x WHERE { ".
        "<$inst> a ?x . ?x rdfs:subClassOf <$this->instClass> }";
      $err = NULL;
      $entries = $engine->executeQueryJSON( $endpoint, $query, $err );
      $this->values = array();
      if(is_array($entries) && count($entries) > 0) {
        foreach($entries as $entry) {
          $this->values[] = $entry["x"];
        }
      }
      return $this->values;
    } else {
      return array(array($inst, "a", $this->getVar()),
                   array($this->getVar(), "http://www.w3.org/2000/01/rdf-schema#subClassOf", $this->instClass));
    }
  }

  public function postpopulate($inst, $entries) {
    $var = substr($this->getVar(),1);
    $this->values = array();
    foreach($entries as $entry) {
      $this->values[] = $entry[$var];
    }
  }

  public function processParams(&$params) {
    $success = parent::processParams($params);
    if(isset($params["multiple"]) &&
       $params["multiple"]=="multiple") {
      $this->multiple = true;
    }
    if(isset($params["size"]))
      $this->size = $params["size"];
    return $success;
  }

  public function render() {
    $res = '<select class="twforms-subclass-select" name="'.$this->id.($this->multiple || $this->isArray ? "[]" : "").'"';
    if($this->multiple)
      $res .= ' multiple="multiple"';
    if($this->size)
      $res .= ' size="'.$this->size.'"';
    $res .= ">";
    $res .= $this->subclassBasedSelect($this->instClass);
    $res .= "</select>";
    return $res;
  }

  public function init($space) {
    /*
    return
      $space.'var self = $(\'select[name="'.$this->id.($this->multiple ? "[]" : "").'"]\',this);'.EOL.
      $space.'TWForms.subclassBasedSelect(self,"'.$this->instClass.'");';
    */
    return "";
  }

  public function dump($spaces)
  {
      $this->getLogger()->logStatus( "$spaces SubclassComponent:" ) ;
      $newspaces = $spaces . "&nbsp;&nbsp;&nbsp;&nbsp;" ;
      parent::dump( $newspaces ) ;

      if( $this->multiple ) $this->getLogger()->logStatus( "$newspaces multiple = true" ) ;
      else $this->getLogger()->logStatus( "$newspaces multiple = true" ) ;

      if( $this->size != null ) $this->getLogger()->logStatus( "$newspaces size = $this->size" ) ;
  }
};
