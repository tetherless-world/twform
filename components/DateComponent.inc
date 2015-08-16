<?php

class DateComponent extends SimpleComponent {
  protected $value = NULL;

  public function __construct() {
    TWForm::registerNS("time", "http://www.w3.org/2006/time#");
    $this->type = "date";
  }

  public function validate() {
    if($this->required) {
      if($this->value=="") {
        return array("Field '".$this->id."' cannot be left blank.");
      }
    } else {
      if(1!=preg_match("-?[0-9]{4}-[0-9]{2}-[0-9]{2}",$this->value)) {
        return array("The value '".$this->value."' is not valid for field '".
          $this->id."'.");
      } else {
        $date = date_parse($this->value);
        if(!$date) {
          return array("The value '".$this->value."' is not valid for field '".
            $this->id."'.");
        }
        if($date["warning_count"]+$date["error_count"]>0) {
          return array("The value '".$this->value."' is not valid for field '".
            $this->id."'.");
        }
      }
    }
    return array();
  }

  public function getValue() {
    return $this->value;
  }

  public function toSPARUL($node) {
    if(!$this->value) return "";
    if($this->rel) {
      return "$node <$this->rel> [ a time:Instant ; time:inXSDDateTime \"$this->value\"^^xsd:dateTime ] .";
    } else if($this->rev) {
      return "[ a time:Instant ; time:inXSDDateTime \"$this->value\"^^xsd:dateTime ] <$this->rel> $node .";
    } else {
      user_error("Neither rel nor rev specified on Date component $this->id", E_USER_WARNING);
      return "";
    }
  }

  public function prepopulate($inst, $top) {
    if($top) {
      $engine = TWSparql::getEngine();
      $endpoint = $engine->getEndpoint();
      $err = NULL;
      if($this->rel) {
        $query = "SELECT ?x WHERE { <$inst> <$this->rel> [ <http://www.w3.org/2006/time#inXSDDateTime> ?x ] }";
      } else if($this->rev) {
        $query = "SELECT ?x WHERE { [ <http://www.w3.org/2006/time#inXSDDateTime> ?x ] <$this->rev> <$inst> }";
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
        $blank = $this->getBlank();
        return array(array($inst, $this->rel, $blank),
                     array($blank, "http://www.w3.org/2006/time#inXSDDateTime",
                           $this->getVar()));
      } else if($this->rev) {
        $blank = $this->getBlank();
        return array(array($blank, "http://www.w3.org/2006/time#inXSDDateTime",
                           $this->getVar()),
                     array($blank, $this->rev, $inst));
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
    return $success;
  }

  public function render() {
    $res = '<input name="'.$this->id.'" type="text" '.
      ($this->value != NULL ? 'value="'.
       substr($this->value,0,10).'" ' : '').'/>';
    return $res;
  }

  public function init($space) {
    return
      $space.'var self = $(\'input[name="'.$this->id.'"]\',this);'.EOL.
      $space.'self.datepicker({dateFormat:"yy-mm-dd"});';
  }

  public function dump($spaces)
  {
      $this->getLogger()->logStatus( "$spaces DateComponent:" ) ;
      $newspaces = $spaces . "&nbsp;&nbsp;&nbsp;&nbsp;" ;
      parent::dump( $newspaces ) ;
  }
};
