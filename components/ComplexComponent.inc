<?php

class CompoundComponent extends Component {
  protected $content=null;
  protected $more=null;
  protected $order=null;
  protected $components=array();
  protected $structure="";
  protected $items=null;
  protected $typeof=null;

  public function __construct() {
    $this->type = "compound";
  }
  
  public function getValue() {
    
  }
  
  private function implodeSparqlBgp($parts) {
    $res = "";
    foreach($parts as $triple) {
      switch($triple[0]) {
      case "optional":
        $res .= "OPTIONAL { ";
        unset($triple[0]);
        $this->implodeSparqlBgp($triple);
        $res .= "} ";
        break;
      case "union":
        unset($triple[0]);
        $first = TRUE;
        foreach($triple as $bgp) {
          if($first) {
            $first = FALSE;
            $res .= "{ ";
          } else {
            $res .= " UNION { ";
          }
          $res .= $this->implodeSparqlBgp($bgp);
          $res .= " } ";
        }
        break;
      default:
        foreach($triple as $key=>$val) {
          if(0!==strpos($val,"?")&&$val!="a")
            $triple[$key] = "<$val>";
        }
        $res .= implode(" ",$triple) . " . ";
      }
    }
    return $res;
  }

  private function implodeSparql($parts) {
    $res = "SELECT * WHERE { ";
    $res .= $this->implodeSparqlBgp($parts);
    $res .= " }";
    return $res;
  }

  public function prepopulate($inst, $top) {
    $var = $this->getVar();
    $res = NULL;
    if($this->rel) {
      $res = array(array($inst, $this->rel, $var),
                   array($var, "a", $this->typeof));
    } else if($this->rev) {
      $res = array(array($var, $this->rev, $inst),
                   array($var, "a", $this->typeof));
    }
    foreach($this->components as $component) {
      if($component instanceof Component) {
        $part = $component->prepopulate($var, FALSE);
        if(is_array($part))
          $res = array_merge($res, $part);
      }
    }
    $str = $this->implodeSparql($res);
    $engine = TWSparql::getEngine();
    $endpoint = $engine->getEndpoint();
    $err = NULL;
    $entries = $engine->executeQueryJSON( $endpoint, $str, $err );
    $var = substr($var,1);
    $processed_entries = array();
    foreach($entries as $entry) {
      $bn = $entry[$var];
      if(!isset($processed_entries[$bn]))
        $processed_entries[$bn] = array();
      $processed_entries[$bn][] = $entry;
    }
    if(count($processed_entries)>0)
      $this->items = array();

    foreach($processed_entries as $node=>$entry) {
      $this->items[$node] = array();
      foreach($this->components as $component) {
        if($component instanceof Component) {
          $x = clone $component;
          $x->postpopulate($this->getVar(), $entry);
        }
        else {
          $x = $component;
        }
        $this->items[$node][] = $x;
      }
    }

    return NULL;
  }

  public function postpopulate($inst, $entries) {
    // Compounds cannot be nested in compounds; this should never be called
  }

  public function processParams(&$params) {
    global $twforms_namespaces;

    $success = parent::processParams($params);

    if(!isset($params["typeof"])) {
      $this->logger->logError("No type specified for compound object.");
      return false;
    }
    $this->typeof = $this->resolve($params["typeof"]);
    if(isset($params["more"])) {
      $this->more = $params["more"];
      $this->setIsArray( true );
    }
    if(isset($params["order"]))
      $this->order = $this->resolve($params["order"]);
    
    // Process the body of the compound unit
    $content = $params["#content"];
    $start = 0;
    $newContent = "";
    $matches = array();
    
    while(strlen($content)>0) {
      if(0<preg_match("/^<twformfield(?P<attrs>([^\/>]*(\/[^>])?)*)(\/>|>((?P<body>[^<]*(<[^\/])?)*)<\/twformfield>)/",$content,$matches,PREG_OFFSET_CAPTURE)) {
        
        $params = TWForm::extractAttrs($matches["attrs"][0]);
        $comp = TWFormUtil::createComponent($params["type"], $this->form, $this->logger);
        $comp->processParams($params);
        
        //checking for isArray, and passing to subfields
        if ($this->getIsArray()) $comp->setIsArray( true );
        
        $comp->setInstClass($this->typeof);
        $comp->setPrefix($this->id."_");
        $this->components[] = $comp;
        $this->structure .= $comp->render_structure();

        $content = substr($content,strlen($matches[0][0]));
      }
      else if(0<preg_match("/^<.*?>/s",$content,$matches,
                           PREG_OFFSET_CAPTURE)) {
        $text = TextNode::parse($matches[0][0]);
        $this->components[] = $text;
        $content = substr($content,strlen($matches[0][0]));
      }
      else if(0<preg_match("/^[^<]+/s",$content,$matches,
                           PREG_OFFSET_CAPTURE)) {
        $text = TextNode::parse($matches[0][0]);
        $this->components[] = $text;
        $content = substr($content,strlen($matches[0][0]));
      }
    }
    $this->items = array($this->components);

    $this->structure .= '<input type="hidden" name="'.$this->id.'_many" value="'.($this->more ? "true" : "false").'"/>';
    $this->structure .= '<input type="hidden" name="'.$this->id.'_order" value="'.($this->order ? $this->order : "false").'"/>';
    $this->structure .= '<input type="hidden" name="'.$this->id.'_class" value="'.$this->typeof.'"/>';
    return $success;
  }

  public function render() {
    $res = '<ul class="compoundfield';
    if($this->more) {
      $res .= " multiple";
    }
    $res .= '">'.EOL;

    foreach($this->items as $item) {
      $res .= "<li class=\"subfield\">";
      foreach($item as $component) {
        if($component instanceof Component) {
          $res .= '<span class="input';
          if ( $component->isRequired() ) {
            $res .= ' required';
          }
          $res .= '">'.$component->render().'</span>';
        } else {
          $res .= $component->render();
        }
      }
      $res .= "</li>".EOL;
    }

    if($this->more) {
      $res .= "<li class=\"more\"><span class=\"link\">".$this->more."</span></li>".EOL;
    }
    $res .= "</ul>".EOL.EOL;
    return $res;
  }

  public function render_structure() {
    return parent::render_structure().$this->structure;
  }

  public function init($space) {
    $content = $space.'var subfields = $("li.subfield span.input",this);'.EOL;
    $counter = 0;
    foreach($this->components as $comp) {
      if($comp instanceof Component) {
        $content .= $space."var subfield=subfields[$counter];".EOL;
        $content .= $space."(function() {".EOL;
        $content .= $comp->init($space."  ").EOL;
        $content .= $space."}).apply( subfield );".EOL;
        $counter++;
      }
    }
    if($this->more) {
      $content .= $space.'var self=this;'.EOL;
      $content .= $space.'var more = $("li.more span.link",this);'.EOL;
      $content .= $space.'more.unbind("click");'.EOL;
      $content .= $space.'more.click(function () {'.EOL;
      $content .= $space.'  TWForms.extendCompoundForm(self);'.EOL;
      $content .= $space.'});'.EOL;
    }
    return $content;
  }

  public function processPOST(&$data, $field, $uri, $obj=NULL) {
    $subfields = array();
    $class = $_POST[$field."_class"];
    foreach($_POST as $subfield => $value) {
      if(0===strpos($subfield,$field."_")) {
        $pos = strpos($subfield,"_type",strlen($field)+1);
        if($pos!==FALSE) {
          $substr = substr($subfield,0,$pos);
          $subfields[$substr] = true;
          $this->logger->logStatus("added subfield " . $substr);
        }
      }
    }

    $values = array();
    $hasvalue = false ;
    foreach($subfields as $subfield => $exists) {
      $this->logger->logStatus("handling subfield " . $subfield);
      $info = array();
      $info["type"] = $_POST[$subfield."_type"];
      $myvalue = $_POST[$subfield];

      // let's see if this compound has any values. If it doesn't, then we don't want to add
      // any data.
      if( is_array( $myvalue ) ) {
        // we want to ignore the subclass, since it will always have a value of at least
        // the top most class in the class tree.
        if( $info["type"] != "subclass" ) {
          // implode will create a single string with all of the values. If that string
          // is empty, then there are no values
          $fullvalue = implode( "", $myvalue ) ;
          if( strlen( $fullvalue ) > 0 ) {
            // hey ... we have values
            $hasvalue = true ;
          }
        }
      }
      else {
        // simple test on a string. If the string has length, then there is a value
        // but, you ask, what if they just hit space a bunch of times.
        $myvalue = trim( $myvalue ) ;
        if( strlen( $myvalue ) > 0 ) {
            $hasvalue = true ;
        }
      }
      $info["value"] = $myvalue;
      $values[$subfield] = $info;
    }

    // we don't want to error out if there aren't any values, but we also don't want to add values
    if( $hasvalue == false ) {
      $this->logger->logStatus("twforms_process_compound_results field '$field' has no values","status");
      return true;
    }

    if($_POST[$field."_many"]=="true") {
      $order = $_POST[$field."_order"];
      if($order == "false") $order = null;
      $len = 0;
      foreach($values as $subfield => $bits) {
        $len = count($bits["value"]);
        break;
      }
      for($i=0;$i<$len;$i++) {
        $self = TWFormUtil::getBnode();
        TWFormUtil::setProperty($data, $self,
          "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
          $class, "uri");
        foreach($values as $subfield => $bits) {
          $value = $bits["value"][$i];
          $subtype = $_POST[$subfield."_type"];
          $this->logger->logStatus("Handling subfield " . $subfield . " of type " . $subtype);
          $component = TWFormUtil::createComponent($subtype, $this->form, $this->logger);
          if(!$component) {
            $this->logger->logStatus("compound processPOST returned empty sub component for subfield '$subfield' of compound element '$field'.".  "error");
            return false;
          }
          if(!$component->processPOST($data, $subfield, $self, $value)) {
            $this->logger->logStatus("processField returned false for subfield '$subfield' of compound element '$field'.","error");
            return false;
          }
        }
        if($order)
          TWFormUtil::setProperty($data, $self, $order, $i+1, "literal",
                  "http://www.w3.org/2001/XMLSchema#int");
        $rel = TWFormUtil::valueOfFieldRel($field);
        $rev = TWFormUtil::valueOfFieldRev($field);
        if($rel) {
          TWFormUtil::setProperty($data, $uri, $rel, $self, "bnode");
        }
        else if($rev) {
          TWFormUtil::setProperty($data, $self, $rev, $uri, "uri");
        }
        else {
          $this->logger->logStatus("Compound form '$field' not related to parent object by rel nor rev attribute.","error");
          return false;
        }
      }
    }
    else {
      $self = TWFormUtil::getBnode();
      foreach($values as $subfield => $bits) {
        $subtype = $_POST[$subfield."_type"];
        $component = TWFormUtil::createComponent($subtype, $this->form, $this->logger);
        if(!$component) {
          $this->logger->logStatus("compound processPOST returned empty sub component for subfield '$subfield' of compound element '$field'.".  "error");
          return false;
        }
        if(!$component->processPOST($data, $subfield, $self, null))
          return false;
      }
      $rel = TWFormUtil::valueOfFieldRel($field);
      $rev = TWFormUtil::valueOfFieldRev($field);
      if($rel) {
        TWFormUtil::setProperty($data, $uri, $rel, $self, "bnode");
      }
      else if($rev) {
        TWFormUtil::setProperty($data, $self, $rev, $uri, (0===strpos($uri,"_:") ? "bnode" : "uri"));
      }
      else {
        $this->logger->logStatus("Compound form '$field' not related to parent object by rel nor rev attribute.","error");
        return false;
      }
    }
    return true;
  }

  public function dump($spaces)
  {
      $this->logger->logStatus( "$spaces ComplexComponent:" ) ;
      $newspaces = $spaces . "&nbsp;&nbsp;&nbsp;&nbsp;" ;
      parent::dump( $newspaces ) ;
  }
};

