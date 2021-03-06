<?php

class DateRangeComponent extends SimpleComponent {
  protected $value = NULL;
  protected $wrap=TRUE;

  public function __construct() {
    $this->type = "daterange";
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
        $query = "PREFIX time: <http://www.w3.org/2006/time#> SELECT ?x ?y WHERE { <$inst> <$this->rel> [ time:hasBeginning [ time:inXSDDateTime ?x ] ; time:hasEnd [ time:inXSDDateTime ?y ] ] }";
      } else if($this->rev) {
        $query = "PREFIX time: <http://www.w3.org/2006/time#> SELECT ?x ?y WHERE { [ time:hasBeginning [ time:inXSDDateTime ?x ] ; time:hasEnd [ time:inXSDDateTime ?y ] ] <$this->rev> <$inst> }";
      } else {
        return NULL;
      }
      $entries = $engine->executeQueryJSON( $endpoint, $query, $err );
      if(is_array($entries) && count($entries) > 0) {
        $this->value = array("start" => $entries[0]["x"],
                             "end" => $entries[0]["y"]);
      }
      return $this->value;
    } else {
      if($this->rel) {
        $blank1 = $this->getBlank();
        $blank2 = $this->getBlank();
        $blank3 = $this->getBlank();
        return array(array($inst, $this->rel, $blank1),
              array("union",
              array(array($blank1,
                    "http://www.w3.org/2006/time#hasBeginning",
                    $blank2),
              array($blank2,
                    "http://www.w3.org/2006/time#inXSDDateTime",
                    $this->getVar()),
              array($blank1,
                    "http://www.w3.org/2006/time#hasEnd",
                    $blank3),
              array($blank3,
                    "http://www.w3.org/2006/time#inXSDDateTime",
                    $this->getVar(1))),
              array(array($blank1,
                    "http://www.w3.org/2006/time#hasBeginning",
                    $blank2),
              array($blank2,
                    "http://www.w3.org/2006/time#inXSDDateTime",
                    $this->getVar())),
              array(array($blank1,
                    "http://www.w3.org/2006/time#hasEnd",
                    $blank3),
              array($blank3,
                    "http://www.w3.org/2006/time#inXSDDateTime",
                    $this->getVar(1)))));
      } else if($this->rev) {
              array($this->getVar(), $this->rev, $inst);
      } else {
        return NULL;
      }
    }
  }

  public function postpopulate($inst, $entries) {
    $startvar = substr($this->getVar(),1);
    $endvar = substr($this->getVar(1),1);
    $this->value = array("start" => $entries[0][$startvar],
                         "end" => (isset($entries[0][$endvar]) ?
                         $entries[0][$endvar] : ""));
  }

  public function processParams(&$params) {
    $success = parent::processParams($params);
    if(isset($params["wrap"])) {
      if(strtoupper($params["wrap"])=="FALSE") {
        $this->wrap = false;
      }
    }
    return $success;
  }

  public function render() {
    $res = 'Start: <input name="'.$this->id.'_start" type="text" '.
      ($this->value != NULL ?
       ' value="'.substr($this->value["start"],0,10).'" '
       :'').'/> ';
    if($this->wrap)
      $res .= '<br/>';
    $res .= 'End: <input name="'.$this->id.'_end" type="text" '.
      ($this->value != NULL ?
       ' value="'.substr($this->value["end"],0,10).'" '
       :'').'/>';
    return $res;
  }

  public function init($space) {
    return
      $space.'var self = $(\'input[name="'.$this->id.'_start"]\',this);'.EOL.
      $space.'self.datepicker({dateFormat:"yy-mm-dd"});'.EOL.
      $space.'self = $(\'input[name="'.$this->id.'_end"]\',this);'.EOL.
      $space.'self.datepicker({dateFormat:"yy-mm-dd"});';
  }

  public function processPOST(&$data, $field, $uri, $obj=NULL) {
    $dtype = null;
    $start = $_POST[$field."_start"];
    $end = $_POST[$field."_end"];
    $this->rel = TWFormUtil::valueOfFieldRel($field);
    $this->rev = TWFormUtil::valueOfFieldRev($field);
    if($start) {
      if(FALSE===strpos($start,"T"))
        $start .= "T04:00:00".TWForm::getTimezone($start);
      $obj = TWFormUtil::getBnode();
      $type = "bnode";
      TWFormUtil::setProperty($data, $obj,
        "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
        "http://www.w3.org/2006/time#Interval", "uri");
      $startNode = TWFormUtil::getBnode();
      TWFormUtil::setProperty($data, $obj,
        "http://www.w3.org/2006/time#hasBeginning",
        $startNode, "bnode");
      TWFormUtil::setProperty($data, $startNode,
        "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
        "http://www.w3.org/2006/time#Instant", "uri");
      TWFormUtil::setProperty($data, $startNode,
        "http://www.w3.org/2006/time#inXSDDateTime",
        $start, "literal",
        "http://www.w3.org/2001/XMLSchema#dateTime");
      if($end) {
        if(FALSE==strpos($end,"T"))
          $end .= "T23:59:59".TWForm::getTimezone($start);
        $endNode = TWFormUtil::getBnode();
        TWFormUtil::setProperty($data, $obj,
                "http://www.w3.org/2006/time#hasEnd",
                $endNode, "bnode");
        TWFormUtil::setProperty($data, $endNode,
                "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
                "http://www.w3.org/2006/time#Instant", "uri");
        TWFormUtil::setProperty($data, $endNode,
                "http://www.w3.org/2006/time#inXSDDateTime",
                $end, "literal",
                "http://www.w3.org/2001/XMLSchema#dateTime");
      }
    } else {
      return true;
    }
    return parent::postProcessPOST($data, $uri, $obj, $type, $dtype);
  }

  public function dump($spaces)
  {
      $this->getLogger()->logStatus( "$spaces DateRangeComponent:" ) ;
      $newspaces = $spaces . "&nbsp;&nbsp;&nbsp;&nbsp;" ;
      parent::dump( $newspaces ) ;

      if( $this->wrap ) $this->getLogger()->logStatus( "$newspaces wrap = true" ) ;
      else $this->getLogger()->logStatus( "$newspaces wrap = false" ) ;
  }
}
