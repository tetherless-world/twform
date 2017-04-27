<?php

class FileComponent extends SimpleComponent {
  protected $value = NULL;

  public function __construct() {
    $this->type = "file";
  }
  
  public function getValue() {
    return $this->value;
  }
  
  public function toSPARUL($node) {
    if(!$this->value) return "";
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
    }
    else {
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
    return $success;
  }

  public function render() {
    $res = '<span><input type="radio" name="'.$this->id.'_method" value="internal" '.($this->value == NULL ? 'checked="checked" ' : '').'/> New File: ';
    $res .= '<upload id="'.$this->id.'_internal"/></span><br/>';
    $res .= '<span><input type="radio" name="'.$this->id.'_method" value="external" '.($this->value != NULL ? 'checked="checked"' : '').'/> Existing URL: ';
    $res .= '<input type="text" name="'.$this->id.'_external" '.($this->value != NULL ? 'value="'.$this->value.'" ' : 'disabled="disabled" ').'/></span>';
    return $res;
  }

  public function init($space) {
    return
      $space.'var radios = $(\'input[name="'.$this->id.'_method"]\');'.EOL.
      $space.'var upload = $(\'input[id="'.$this->id.'_internal"]\');'.EOL.
      $space.'var url = $(\'input[name="'.$this->id.'_external"]\');'.EOL.
      $space.'TWForms.fileUploadRadios(radios,upload,url);';
  }

  public function processPOST(&$data, $field, $uri, $obj=NULL) {
    $dtype = null;
    if(!$obj) {
      $obj = TWFormUtil::valueOfFileField($field);
    }
    $this->rel = TWFormUtil::valueOfFieldRel($field);
    $this->rev = TWFormUtil::valueOfFieldRev($field);
    $type = "uri";
    return parent::postProcessPOST($data, $uri, $obj, $type, $dtype);
  }

  public function dump($spaces)
  {
      $this->getLogger()->logStatus( "$spaces FileComponent:" ) ;
      $newspaces = $spaces . "&nbsp;&nbsp;&nbsp;&nbsp;" ;
      parent::dump( $newspaces ) ;
  }
};
