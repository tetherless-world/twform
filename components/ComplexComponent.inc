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
  
  public function toSPARUL($node) {
    
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
    //drupal_set_message(htmlspecialchars($str));
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
      drupal_set_message("No type specified for compound object.","error");
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
        $comp = $this->getForm()->createComponent($params, $twforms_namespaces);
        
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
      $content .= $space.'more.click(function () {'.EOL;
      $content .= $space.'  TWForms.extendCompoundForm(self);'.EOL;
      $content .= $space.'});'.EOL;
    }
    return $content;
  }

  public function dump($spaces)
  {
      $this->getLogger()->logStatus( "$spaces ComplexComponent:" ) ;
      $newspaces = $spaces . "&nbsp;&nbsp;&nbsp;&nbsp;" ;
      parent::dump( $newspaces ) ;
  }
};

