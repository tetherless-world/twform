<?php

class TextareaComponent extends SimpleComponent {
  protected $rows=10;
  protected $cols=60;
  protected $value=NULL;

  public function __construct() {
    $this->type = "textarea";
  }

  public function getValue() {
    return $this->value;
  }

  public function toSPARUL($node) {
    if($this->rel) {
      // TODO(ewpatton) escape $node """ becomes ""\"
      $dodo = "$node <$this->rel> \"\"\"<div xmlns=\"http://www.w3.org/1999/xhtml\">$this->value</div>\"\"\" .";
      $this->getLogger()->logStatus( "thigy = $dodo" ) ;
      return "$node <$this->rel> \"\"\"<div xmlns=\"http://www.w3.org/1999/xhtml\">$this->value</div>\"\"\" .";
    } else if($this->rev) {
      user_error("Rev shouldn't be specified on a textarea component.",
                 E_USER_WARNING);
      return "";
    } else {
      user_error("Neither rel nor rev specified on Textarea component $this->id", E_USER_WARNING);
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
        $matches = array();
        if ( preg_match( "/\\A\\s*<div.*>(.*)<\\/div>\\s*\\Z/is",
                         $this->value, $matches ) ) {
          $this->value = $matches[1];
        }
      }
      return $this->value;
    } else {
      if($this->rel) {
        array(array($inst, $this->rel, $this->getVar()));
      } else if($this->rev) {
        array(array($this->getVar(), $this->rev, $inst));
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
    if(isset($params["rows"])) {
      $this->rows = $params["rows"];
    }
    if(isset($params["cols"])) {
      $this->cols = $params["cols"];
    }
    return $success;
  }

  public function render() {
    $res = '<textarea name="'.$this->id.'" rows="'.$this->rows.'" cols="'.$this->cols.'">'.($this->value != NULL ? $this->value : "").'</textarea>';
    return $res;
  }

  public function init($space) {
    return "";
  }

  public function processPOST(&$data, $field, $uri, $obj=NULL) {
    if(!$obj) {
      $obj = $_POST[$field];
    }
    $obj = "<div xmlns=\"http://www.w3.org/1999/xhtml\">".$obj."</div>";
    $this->rel = TWFormUtil::valueOfFieldRel($field);
    $this->rev = TWFormUtil::valueOfFieldRev($field);
    $type = "literal";
    $dtype = "http://www.w3.org/1999/02/22-rdf-syntax-ns#XMLLiteral";
    return parent::postProcessPOST($data, $uri, $obj, $type, $dtype);
  }

  public function dump($spaces)
  {
      $this->getLogger()->logStatus( "$spaces TextareaComponent:" ) ;
      $newspaces = $spaces . "&nbsp;&nbsp;&nbsp;&nbsp;" ;
      parent::dump( $newspaces ) ;

      $this->getLogger()->logStatus( "$newspaces rows = $this->rows" ) ;
      $this->getLogger()->logStatus( "$newspaces cols = $this->cols" ) ;
  }
};
