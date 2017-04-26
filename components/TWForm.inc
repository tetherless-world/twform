<?php

include_once("Component.inc");
include_once("ComplexComponent.inc");
include_once("SimpleComponent.inc");
include_once("UrlComponent.inc");
include_once("TextComponent.inc");
include_once("EmailComponent.inc");
include_once("FileComponent.inc");
include_once("AliasComponent.inc");
include_once("InstanceComponent.inc");
include_once("SubclassComponent.inc");
include_once("DateComponent.inc");
include_once("DateRangeComponent.inc");
include_once("TextareaComponent.inc");
include_once("TextNode.inc");
include_once("TWFormUtil.inc");

/**
 * Defines a logger object for TWForms to provide feedback to users.
 * @author pattoe
 */
interface TWFormLogger {
  /**
   * Return whether debugging is turned on or not
   */
  public function shouldDebug();

  /**
   * Set whether or not we should debug
   */
  public function enableDebug($val);

  /**
   * Logs a string as an error.
   * @param string $str String to log to the error console.
   */
  public function logError($str);
  /**
   * Logs a string as a warning
   * @param string $str String to log to the warning console.
   */
  public function logWarning($str);
  /**
   * Logs a string as a status
   * @param string $str String to log to the status console.
   */
  public function logStatus($str);

  /**
   * Logs a string at the given log level
   * @param int $level Constant from TWFormLogLevel
   * @param string $str String to log to the console.
   */
  public function log($level, $str);

  public function setLogLevel($level);
  public function getLogLevel();
};


class TWFormLogLevel {
  const Error = 0;
  const Warn = 1;
  const Info = 2;
  const Debug = 3;
  const Trace = 4;
};

/**
 * Default logger class that logs to PHP error console.
 * This should be superceded by a CMS-specific logger implementation.
 *
 * @author pattoe
 */
class PHPTWFormLogger implements TWFormLogger {
  public $debugFlag = false;

  /**
   * Return whether debugging is turned on or not
   */
  public function shouldDebug() {
    return $this->$debugFlag;
  }

  /**
   * Set whether or not we should debug
   */
  public function enableDebug($val) {
    $this->$debugFlag = $val;
  }

  /**
   * Logs an error to the console.
   * @param string $str String to log to the error console.
   */
  public function logError($str) {
	if( $this->shouldDebug() ) {
        trigger_error($str, E_USER_ERROR);
    }
  }

  /**
   * Logs a warning to the console.
   * @param string $str String to log to the warning console.
   */
  public function logWarning($str) {
	if( $this->shouldDebug() ) {
        trigger_error($str, E_USER_WARNING);
    }
  }

  /**
   * Logs a status to the console.
   * @param string $str String to log to the status console.
   */
  public function logStatus($str) {
	if( $this->shouldDebug() ) {
        trigger_error($str, E_USER_NOTICE);
    }
  }

  private $logLevel = TWFormLogLevel::Warn;

  /**
   * Logs a message to the console
   */
  public function log($level, $str) {
    if($level <= $this->logLevel) {
      if($level == TWFormLogLevel::Error) {
        $this->logError($str);
      }
      else if($level == TWFormLogLevel::Warn) {
        $this->logWarn($str);
      }
      else {
        $this->logStatus($str);
      }
    }
  }

  public function setLogLevel($level) {
    $this->logLevel = $level;
  }

  public function getLogLevel() {
    return $this->logLevel;
  }
};

/**
 * Core of the TWForms functionality. This must be subclassed to provide
 * CMS-specific code.
 *
 * @author pattoe
 */
abstract class TWForm {
  protected static $logger;
  public static $editing = FALSE;
  public static $namespaces =
      array("rdf"=>"http://www.w3.org/1999/02/22-rdf-syntax-ns#",
            "rdfs"=>"http://www.w3.org/2000/01/rdf-schema#",
            "time"=>"http://www.w3.org/2006/time#",
            "owl"=>"http://www.w3.org/2002/07/owl#",
            "foaf"=>"http://xmlns.com/foaf/0.1/",
            "tw"=>"http://tw.rpi.edu/schema/",
            "dc"=>"http://purl.org/dc/terms/",
            "skos"=>"http://www.w3.org/2008/05/skos#",
            "xsd"=>"http://www.w3.org/2001/XMLSchema#");
  public $class = NULL;
  public $contentType = NULL;
  public $body = array();
  public $submit = "Save Instance";
  protected $bodyContent = null;
  protected $fields = array();
  protected $data = NULL;
  protected $uri = NULL;
  private static $simulate=TRUE;
  protected $header=null;
  protected $footer=null;
  private static $headerTag =
      "<twformheader(([^\/>]*(\/[^>])?)*)(\/>|>(([^<]*(<[^\/])?)*)<\/twformheader>)";

  /**
   * URL used in the action parameter of an HTML form.
   * @return string
   */
  protected abstract function getAction();

  /**
   * URI of the RDF instance being edited by this form.
   * @return string
   */
  protected abstract function getInstanceUri();

  /**
   * Returns a Logger object for reporting information about the form processing.
   * @return TWFormLogger
   */
  protected abstract function getLogger();

  /**
   * Returns the base path of the underlying CMS
   * @return string
   */
  protected abstract function getBasePath();

  /**
   * Gets the alternate base path for alias components
   * @return string
   */
  public abstract function getAltBase();

  /**
   * Returns whether or not the forms module should simulate commands rather
   * than execute them.
   * @return boolean
   */
  protected abstract function shouldSimulate();

  /**
   * Creates a page in the underlying CMS
   * @param string $title Title for the page
   * @param string $content Body of the page
   */
  protected abstract function createPage($title, $content);

  /**
   * Gets the ID for the form. Should be unique in the underlying CMS.
   * @return string
   */
  protected abstract function getId();

  /**
   * Register a namespace for a URI.
   *
   * <code>TWForm::registerNS("ex","http://example.com/#");</code>
   *
   * @param string $ns Abbreviation for the namespace
   * @param string $uri Absolute URI represented by $ns
   */
  public static function registerNS($ns,$uri) {
    TWForm::$namespaces[$ns] = $uri;
  }

  /**
   * Returns the PREFIX portion of a SPARQL Query
   *
   * @return string SPARQL preamble
   */
  public static function getQueryHeader() {
    $res = "";
    foreach(TWForm::$namespaces as $ns=>$uri) {
      $res .= "PREFIX $ns: <$uri>\r\n";
    }
    return $res;
  }

  /**
   * Expands a CURIE using the current set of namespaces.
   *
   * <code>TWForm::expandCURIE("rdf:type")</code>
   * => "http://www.w3.org/1999/02/22-rdf-syntax-ns#type"
   *
   * @param string $str Valid CURIE
   * @return string Absolute URI represented by CURIE
   */
  public static function expandCURIE($str) {
    $parts = explode(":",$str);
    return TWSparql::rfc2396(TWForm::$namespaces[$parts[0]],$parts[1]);
  }

  /**
   * Generates an associative array from XML attributes.
   *
   * <code>TWForm::extractAttrs('xmlns="http://example.com/" rel="dc:title"/>');</code>
   * => array("xmlns"=>"http://example.com/","rel"=>"dc:title")
   *
   * @param string $str A string of XML attributes
   * @return array Associative array mapping XML attribute to its value
   */
  public static function extractAttrs($str) {
    $attrs = array();
    $matches = array();
    if(0<preg_match_all("/\s*([^=]*)=\"([^\"]*)\"/", $str, $matches,
        PREG_SET_ORDER)) {
      foreach($matches as $data) {
      	$attrs[$data[1]] = $data[2];
      }
    }
    return $attrs;
  }

  public static function extractClasses($content) {
    $classes = NULL;
    $matches = NULL;
    if(0<preg_match_all("/".TWForm::$headerTag."/", $content, $matches)) {
      foreach($matches[1] as $match) {
        $attrs = TWForm::extractAttrs($match);
        foreach($attrs as $key => $value) {
          if(strpos($key, "xmlns:")) {
            TWForm::registerNS(substr($key, 6), $value);
          }
        }
        if($attrs["class"]) {
          if($classes == NULL) $classes = array();
          $classes[] = TWForm::expandCURIE($attrs["class"]);
        }
      }
    }
    return $classes;
  }

  /**
   * Processes a <twformheader> element for the given node.
   * @param array $matches Matches array populated by preg_match
   * @return string HTML fragment for the header element
   */
  protected function processHeader($matches) {
    $params = TWForm::extractAttrs($matches[1][0]);
    foreach($params as $param => $value) {
      if(0===strpos($param,"xmlns:")) {
        $ns = substr($param,6);
        TWForm::$namespaces[$ns] = $value;
      }
    }
    $this->class = TWForm::expandCURIE($params["class"]);
    $res = '<form name="twform" method="POST" action="'.$this->getAction().'" onsubmit="return TWForms.validate(this);">'.EOL;
    $res .= '<input type="hidden" name="twform_id" value="'.$this->getId().'"/>'.EOL;
    $res .= '<input type="hidden" name="class" value="'.$this->class.'" />'.EOL;
    $res .= '<input type="hidden" name="contenttype" value="'.$params["type"].'"/>'.EOL;
    return $res;
  }

  /**
   * Processes a <twformbody> element for the given node.
   * @param array $matches Matches array populated by preg_match
   * @return string HTML fragment for the body element
   */
  protected function processBody($matches) {
    $this->bodyContent = base64_encode($matches[5][0]);
    $res = '<input type="hidden" name="body" value="'.$this->bodyContent.'"/>';
    return $res;
  }

  /**
   * Processes a <twformsubmit> element for the given node.
   * @param array $matches Matches array populated by preg_match
   * @return string HTML fragment for the submit element
   */
  protected function processSubmit($matches) {
    $params = TWForm::extractAttrs($matches[1][0]);
    if ( $this->getInstanceUri() == '$uri' ) {
      $res = '<input type="submit" value="'.$params["title"].'" />'.EOL;
    } else {
      $res = '<input type="hidden" value="'.$this->getInstanceUri().'" name="uri" />'.EOL;
      $res .= '<input type="submit" value="'.str_replace("Create", "Save", $params["title"]).'" />'.EOL;
    }
    $res .= '</form>'.EOL;
    return $res;
  }

  /**
   * Parses a form from the text. This function expects that $content begins with a <twformheader> element and ends
   * with a <twformsubmit> element.
   * @param string $content String used to construct form object.
   */
  protected function parse($content) {
    // Parse form
    while(strlen($content)>0) {
      $matches = array();

      // Handle header construct
      if(0<preg_match("/^<twformheader(([^\/>]*(\/[^>])?)*)(\/>|>(([^<]*(<[^\/])?)*)<\/twformheader>)/s",
                    //"/ <twformheader(([^\/>]*(\/[^>])?)*)(\/>|>(([^<]*(<[^\/])?)*)<\/twformheader>)/"
          $content,$matches,PREG_OFFSET_CAPTURE)) {
        $this->header = $this->processHeader($matches);
        $content = substr($content,strlen($matches[0][0]));
      }
      // Handle body construct
      else if(0<preg_match("/^<twformbody(([^\/>]*(\/[^>])?)*)(\/>|>(.*)<\/twformbody>)/s",
          $content,$matches,PREG_OFFSET_CAPTURE)) {
        $text = TextNode::parse($this->processBody($matches));
        $this->body[] = $text;
        $content = substr($content,strlen($matches[0][0]));
      }
      // Handle compound construct
      else if(0<preg_match("/^<twformcompound(?P<attrs>([^\/>]*(\/[^>])?)*)(\/>|>(?P<body>([^<]*?(<[^\/])?)*?)<\/twformcompound>)/s",
          $content,$matches,PREG_OFFSET_CAPTURE)) {
        $params = TWForm::extractAttrs($matches["attrs"][0]);
        $params["#content"] = $matches["body"][0];
        $params["type"] = "compound";
        $item = TWFormUtil::createComponent("compound", $this, $this->getLogger());
        $item->processParams($params);
        $item->setInstClass($this->class);
        $this->fields[$item->getId()] = $item;
        $this->body[] = $item;
        $content = substr($content,strlen($matches[0][0]));
      }
      // Handle field construct
      else if(0<preg_match("/^<twformfield(?P<attrs>([^\/>]*(\/[^>])?)*)(\/>|>((?P<body>[^<]*(<[^\/])?)*)<\/twformfield>)/",
          $content,$matches,PREG_OFFSET_CAPTURE)) {
        $params = TWForm::extractAttrs($matches["attrs"][0]);
        $item = TWFormUtil::createComponent($params["type"], $this, $this->getLogger());
        $item->processParams($params);
        $item->setInstClass($this->class);
        $this->fields[$item->getId()] = $item;
        if(!TWForm::$editing || $item->getId() != "uri")
          $this->body[] = $item;
        $content = substr($content,strlen($matches[0][0]));
      }
      // Handle submit construct
      else if(0<preg_match("/^<twformsubmit(([^\/>]*(\/[^>])?)*)(\/>|>(([^<]*(<[^\/])?)*)<\/twformsubmit>)/",
          $content,$matches,PREG_OFFSET_CAPTURE)) {
        $this->footer = $this->processSubmit($matches);
        $content = substr($content,strlen($matches[0][0]));
      }
      // Handle non-TWForm tags
      else if(0<preg_match("/^<.*?>/s",$content,$matches,PREG_OFFSET_CAPTURE)) {
        $text = TextNode::parse($matches[0][0]);
        $this->body[] = $text;
        $content = substr($content,strlen($matches[0][0]));
      }
      // Handle text nodes
      else if(0<preg_match("/^[^<]+/s",$content,$matches,PREG_OFFSET_CAPTURE)) {
        $text = TextNode::parse($matches[0][0]);
        $this->body[] = $text;
        $content = substr($content,strlen($matches[0][0]));
      }
      else {
      	$this->getLogger()->logError("TWForm:parse - Unable to determine content to parse.");
      }
    }
    if(TWForm::$editing) {
      $instURI = $this->getInstanceUri();
      $this->prepopulate($instURI);
    }
  }

  /**
   * Prepopulates the form content with existing values. Used to generate edit forms where we want to change existing
   * triples in the triple store.
   * @param string $inst URI of the instance to query
   */
  public function prepopulate($inst) {
    foreach($this->body as $component) {
      if($component instanceof Component) $component->prepopulate($inst, TRUE);
    }
  }

  /**
   * Renders the form as a set of HTML elements
   * @return string HTML form fragment
   */
  public function render() {
    $this->getLogger()->log(TWFormLogLevel::Trace, "Entering TWForm::render");
    $res = $this->header."<div class=\"wrapper\">".EOL;
    foreach($this->body as $elem) {
      $this->getLogger()->log(TWFormLogLevel::Debug,
          htmlspecialchars(print_r($elem, true)));
      if($elem->getType() != "textNode" && $elem->getType() != "compound") {
        $res .= '<div class="field';
        if($elem->isRequired())
          $res .= " required";
        $res .= '"><span class="label';
        $res .= '">';
        $res .= $elem->getTitle();
        if($elem->isRequired())
          $res .= '<span>*</span>';
        $res .= '</span><div class="wrapper">';
        $res .= $elem->render().$elem->render_structure().'</div></div>';
      }
      else if($elem->getType() == "compound") {
        $res .= '<div class="field';
        if($elem->isRequired())
          $res .= " required";
        $res .= '"><span class="label';
        $res .= '">';
        $res .= $elem->getTitle();
        if($elem->isRequired())
          $res .= '<span>*</span>';
        $res .= '</span><div class="wrapper">';
        $res .= $elem->render().$elem->render_structure().'</div></div>';
      }
      else
      	$res .= $elem->render();
    }
    $res .= '<script type="text/javascript">//<![CDATA['.EOL;
    $res .= 'TWForms.endpoint = "'.TWSparql::getEngine()->getEndpoint().'";'.EOL;
    $res .= '$(document).ready(function() {'.EOL;
    $res .= '  var elems = $("div.field",document.twform);'.EOL;
    $counter = 0;
    foreach($this->body as $node) {
      // Since a compound element also has a field class associated with it then we need
      // to add the function for it too
      //if($node->getType() != "textNode" && $node->getType() != "compound") {
      if($node->getType() != "textNode" ) {
        // Initialize form components here
        $res .= "  var elem=elems[$counter];".EOL;
        $res .= "  (function() {".EOL;
        $res .= $node->init('    ').EOL;
        $res .= "  }).apply( elem );".EOL;
        $counter++;
      }
    }
    $res .= '  var elems2 = $("ul.compoundfield",document.twform);'.EOL;
    $counter = 0;
    foreach($this->body as $node) {
      if($node->getType() == "compound") {
        // Initialize compound form components here
        $res .= "  var elem2=elems2[$counter];".EOL;
        $res .= "  (function() {".EOL;
        $res .= $node->init('    ').EOL;
        $res .= "  }).apply( elem2 );".EOL;
        $counter++;
      }
    }
    $res .= "});".EOL;
    $res .= "//]]>".EOL;
    $res .= "</script>".EOL;
    $res .= "</div>".$this->footer.EOL;
    $this->getLogger()->log(TWFormLogLevel::Trace, "Leaving TWForm::render");
    return $res;
  }

  /**
   * Converts the form POST data into an RDF/XML representation
   * @return string RDF/XML document containing form data
   */
  public function toRDF() {
    $nsLookup = array();
    foreach(TWForm::$namespaces as $ns => $uri) {
      $nsLookup[$uri] = $ns;
    }
    $counter = 0;
    $result = "";
    foreach($this->data as $subj => $props) {
      $sSplit = TWForm::segmentURI($subj);
      if($sSplit["ns"]!="_:"&&!isset($nsLookup[$sSplit["ns"]])) {
        $ns = "j.$counter";
        $counter++;
        TWForm::$namespaces[$ns] = $sSplit["ns"];
        $nsLookup[$sSplit["ns"]] = $ns;
      }
      $result .= "<rdf:Description ";
      if($sSplit["ns"]=="_:") {
        $result .= "rdf:nodeID=\"".$sSplit["id"]."\">\r\n";
      } else {
        $result .= "rdf:about=\"".$subj."\">\r\n";
      }
      foreach($props as $prop => $objs) {
        $pSplit = TWForm::segmentURI($prop);
        if(!isset($nsLookup[$pSplit["ns"]])) {
          $ns = "j.$counter";
          $counter++;
          TWForm::$namespaces[$ns] = $pSplit["ns"];
          $nsLookup[$pSplit["ns"]] = $ns;
        }
        foreach($objs as $obj) {
          if($obj["type"]=="uri") {
            $result .= "<".$nsLookup[$pSplit["ns"]].":".$pSplit["id"].
              " rdf:resource=\"".$obj["value"]."\"/>\r\n";
          } else if($obj["type"]=="literal") {
            $result .= "<".$nsLookup[$pSplit["ns"]].":".$pSplit["id"];
            if(isset($obj["datatype"]) && $obj["datatype"] ==
               "http://www.w3.org/1999/02/22-rdf-syntax-ns#XMLLiteral") {
              $result .= " rdf:parseType=\"Literal\"";
            } else if(isset($obj["datatype"])) {
              $result .= " rdf:datatype=\"".$obj["datatype"]."\"";
            } else if(isset($obj["lang"])) {
              $result .= " xml:lang=\"".$obj["lang"]."\"";
            }
            $result .= ">";
            $result .= $obj["value"];
            $result .= "</".$nsLookup[$pSplit["ns"]].":".$pSplit["id"].">\r\n";
          } else if($obj["type"]=="bnode") {
            $result .= "<".$nsLookup[$pSplit["ns"]].":".$pSplit["id"].
              " rdf:nodeID=\"".substr($obj["value"],2)."\"/>\r\n";
          }
        }
      }
      $result .= "</rdf:Description>\r\n";
    }
    $header = "<?xml version=\"1.0\"?><rdf:RDF";
    foreach($nsLookup as $uri => $ns) {
      if($uri!="")
        $header .= " xmlns:$ns=\"$uri\"";
    }
    $header .= ">\r\n";
    $result = $header . $result;
    $result .= "</rdf:RDF>";
    return $result;
  }

  /**
   * Converts the form structure into a SPARQL query to be used to populate the form with content.
   * @return string SPARQL query
   */
  public function toSPARQL() {

  }

  /**
   * Converts the form POST data into a SPARQL 1.1 Update request
   * @return string SPARUL request for updating the triple store
   */
  public function toSPARUL() {
    $output = "";
    foreach($this->data as $s => $pmap) {
      foreach($pmap as $p => $oarr) {
        foreach($oarr as $o) {
          $sb = preg_match("/^_:/",$s); //check if subject is blank
          if ($sb) $output .= "$s <$p> ";
          else $output .= "<$s> <$p> ";
          if ($o['type'] == 'uri') //print URI object
            $output .= '<'.$o['value']."> .\n";
          else if ($o['type'] == 'literal') { //print literal object
            $val = $o['value'];
            $val = preg_replace('/\r/','',$val);
            $val = preg_replace('/\n/','',$val);
            $output .= '"'.preg_replace("/\"/","\\\"",$val).'"';
            if (array_key_exists('datatype',$o)) $output .= '^^<'.$o['datatype'].'>';
            if (array_key_exists('lang',$o)) $output .= '@'.$o['lang'];
            $output .= " .\n";
          } else if ($o['type'] == 'bnode') { //print blank object
            $output .= $o['value']." .\n";
          }
        }
      }
    }
    return $output;
  }

  /**
   * Constructs a timezone string compatible with XSD Datatypes 2nd Edition
   * @param string $dt Datetime object to use for generating the Timezone
   * @param string $tz Timezone to use, defaults to server's timezone (e.g. "America/New_York")
   * @return string Returns the time zone component of an XSD Datetime representation (e.g. "-0500", "+0000")
   */
  public static function getTimezone($dt = NULL, $tz = NULL) {
    if($tz == NULL) $tz = date_default_timezone_get ();
    if($dt == NULL) $dt = "now";
    $here = new DateTimeZone($tz);
    $date = new DateTime($dt,$here);
    $hour = floor($date->getOffset()/3600);
    $minute = ($date->getOffset()%3600)/3600*60;

    $str = "";
    if($hour < 0) {
      $str = "-";
      $hour = -$hour;
    }
    else {
      $str = "+";
    }

    if($hour < 10) $str .= "0".$hour;
    else $str .= $hour;

    $str .= ":";

    if($minute < 10) $str .= "0".$minute;
    else $str .= $minute;

    return $str;
  }

  /**
   * Checks whether the field is marked required in the form.
   * @param string $field Name of field to check
   * @return boolean True if the field is required, false otherwise.
   */
  protected function isFieldRequired($field) {
    if(!isset($this->fields[$field])) {
      return false;
    }
    return $this->fields[$field]->isRequired();
  }

  public static function segmentURI($uri) {
    $i = strlen($uri)-1;
    // Taken from http://www.w3.org/TR/REC-xml-names/#NT-NCName
    while($i>0) {
      if(("A"<=$uri[$i]&&"Z">=$uri[$i])||
        $uri[$i]=="_"||
        ("a"<=$uri[$i]&&"z">=$uri[$i])||
        ("\xC0"<=$uri[$i]&&"\xD6">=$uri[$i])||
        ("\xD8"<=$uri[$i]&&"\xF6">=$uri[$i])||
        ("\xF8"<=$uri[$i]&&"\xFF">=$uri[$i])||
        $uri[$i]=="-"||
        $uri[$i]=="."||
        ("0"<=$uri[$i]&&"9">=$uri[$i])||
        $uri[$i]=="\xB7") {
        $i--;
        continue;
      }
      $i++;
      return array("ns"=>substr($uri,0,$i),"id"=>substr($uri,$i));
    }
    return array("ns"=>"","id"=>$uri);
  }

  protected function processField($field, $uri, $obj=NULL) {
    $rel = $rev = $type = $dtype = null;
    $type = $_POST[$field."_type"];
    switch($type) {
    case "alias":
      $page = TWFormUtil::valueOfAliasField($field, $this->getBasePath());
      if(!$obj)
        $obj = $page;
      $rel = TWFormUtil::valueOfFieldRel($field);
      $rev = TWFormUtil::valueOfFieldRev($field);
      $type = "uri";
      break;
    case "text":
      if(!$obj)
        $obj = $_POST[$field];
      $this->getLogger()->logStatus("Object: $obj");
      $rel = TWFormUtil::valueOfFieldRel($field);
      $rev = TWFormUtil::valueOfFieldRev($field);
      $this->getLogger()->logStatus("Rel: $rel");
      $this->getLogger()->logStatus("Rev: $rev");
      $type = "literal";
      $dtype = "http://www.w3.org/2001/XMLSchema#string";
      break;
    case "daterange":
      $start = $_POST[$field."_start"];
      $end = $_POST[$field."_end"];
      $rel = TWFormUtil::valueOfFieldRel($field);
      $rev = TWFormUtil::valueOfFieldRev($field);
      if($start) {
        if(FALSE===strpos($start,"T"))
          $start .= "T00:00:00".TWForm::getTimezone($start);
        $obj = TWFormUtil::getBnode();
        $type = "bnode";
        TWFormUtil::setProperty($this->data, $obj,
          "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
          "http://www.w3.org/2006/time#Interval", "uri");
        $startNode = TWFormUtil::getBnode();
        TWFormUtil::setProperty($this->data, $obj,
          "http://www.w3.org/2006/time#hasBeginning",
          $startNode, "bnode");
        TWFormUtil::setProperty($this->data, $startNode,
          "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
          "http://www.w3.org/2006/time#Instant", "uri");
        TWFormUtil::setProperty($this->data, $startNode,
          "http://www.w3.org/2006/time#inXSDDateTime",
          $start, "literal",
          "http://www.w3.org/2001/XMLSchema#dateTime");
        if($end) {
          if(FALSE==strpos($end,"T"))
            $end .= "T23:59:59".TWForm::getTimezone($start);
          $endNode = TWFormUtil::getBnode();
          TWFormUtil::setProperty($this->data, $obj,
                  "http://www.w3.org/2006/time#hasEnd",
                  $endNode, "bnode");
          TWFormUtil::setProperty($this->data, $endNode,
                  "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
                  "http://www.w3.org/2006/time#Instant", "uri");
          TWFormUtil::setProperty($this->data, $endNode,
                  "http://www.w3.org/2006/time#inXSDDateTime",
                  $end, "literal",
                  "http://www.w3.org/2001/XMLSchema#dateTime");
        }
      }
      else {
        return $this->data;
      }
      break;
    case "url":
      if(!$obj)
        $obj = $_POST[$field];
      $rel = TWFormUtil::valueOfFieldRel($field);
      $rev = TWFormUtil::valueOfFieldRev($field);
      $type = "uri";
      break;
    case "email":
      if(!$obj)
        $obj = $_POST[$field];
      $obj = "mailto:".$obj;
      $rel = TWFormUtil::valueOfFieldRel($field);
      $rev = TWFormUtil::valueOfFieldRev($field);
      $type = "uri";
      break;
    case "textarea":
      if(!$obj)
        $obj = $_POST[$field];
      $obj = "<div xmlns=\"http://www.w3.org/1999/xhtml\">".$obj."</div>";
      $rel = TWFormUtil::valueOfFieldRel($field);
      $rev = TWFormUtil::valueOfFieldRev($field);
      $type = "literal";
      $dtype = "http://www.w3.org/1999/02/22-rdf-syntax-ns#XMLLiteral";
      break;
    case "instance":
      if(!$obj)
        $obj = $_POST[$field];
      if($obj == "") return $this->data;
      $rel = TWFormUtil::valueOfFieldRel($field);
      $rev = TWFormUtil::valueOfFieldRev($field);
      $type = "uri";
      break;
    case "subclass":
      if(!$obj)
        $obj = $_POST[$field];
      $rel = "http://www.w3.org/1999/02/22-rdf-syntax-ns#type";
      $type = "uri";
      break;
    case "file":
      if(!$obj)
        $obj = TWFormUtil::valueOfFileField($field);
      $rel = TWFormUtil::valueOfFieldRel($field);
      $rev = TWFormUtil::valueOfFieldRev($field);
      $type = "uri";
      break;
    case "date":
      if($obj)
        $val = $obj;
      else
        $val = $_POST[$field];
      if($val=="") return $this->data;
      if(FALSE===strpos($field,"T"))
        $val = $val."T00:00:00".TWForm::getTimezone($val);
      $rel = TWFormUtil::valueOfFieldRel($field);
      $bnode = TWFormUtil::getBnode();
      TWFormUtil::setProperty($this->data, $bnode,
        "http://www.w3.org/2006/time#inXSDDateTime",
        $val, "literal",
        "http://www.w3.org/2001/XMLSchema#dateTime");
      TWFormUtil::setProperty($this->data, $bnode,
        "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
        "http://www.w3.org/2006/time#Instant", "uri");
      $obj = $bnode;
      $type = "bnode";
      break;
    default:
      $this->getLogger()->logStatus("Field '$field' of unknown type '$type'.","warning");
      return false;
    }
    if($rel) {
      if($obj == "") return $this->data;
      TWFormUtil::setProperty($this->data, $uri, $rel, $obj, $type, $dtype);
    }
    else if($rev) {
      if($obj == "") return $this->data;
      if($uri == "") return $this->data;
      TWFormUtil::setProperty($this->data, $obj, $rev, $uri, (0===strpos($uri,"_:") ? "bnode" : "uri"));
    }
    else {
      $this->getLogger()->logStatus("No rel or rev set for input '$field'.","error");
      return false;
    }
    return $this->data;
  }

  protected function processCompound($field, $uri) {
    $subfields = array();
    $class = $_POST[$field."_class"];
    foreach($_POST as $subfield => $value) {
      if(0===strpos($subfield,$field."_")) {
        $pos = strpos($subfield,"_type",strlen($field)+1);
        if($pos!==FALSE) {
          $substr = substr($subfield,0,$pos);
          $subfields[$substr] = true;
        }
      }
    }

    $values = array();
    $hasvalue = false ;
    foreach($subfields as $subfield => $exists) {
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
      $this->getLogger()->logStatus("twforms_process_compound_results field '$field' has no values","status");
      return $this->data;
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
        TWFormUtil::setProperty($this->data, $self,
          "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
          $class, "uri");
        foreach($values as $subfield => $bits) {
          $value = $bits["value"][$i];
          if(!$this->processField($subfield, $self, $value)) {
            $this->getLogger()->logStatus("processField returned false for subfield '$subfield' of compound element '$field'.","error");
            return false;
          }
        }
        if($order)
          TWFormUtil::setProperty($this->data, $self, $order, $i+1, "literal",
                  "http://www.w3.org/2001/XMLSchema#int");
        $rel = TWFormUtil::valueOfFieldRel($field);
        $rev = TWFormUtil::valueOfFieldRev($field);
        if($rel) {
          TWFormUtil::setProperty($this->data, $uri, $rel, $self, "bnode");
        }
        else if($rev) {
          TWFormUtil::setProperty($this->data, $self, $rev, $uri, "uri");
        }
        else {
          $this->getLogger()->logStatus("Compound form '$field' not related to parent object by rel nor rev attribute.","error");
          return false;
        }
      }
    }
    else {
      $self = TWFormUtil::getBnode();
      foreach($values as $subfield => $bits) {
        if(!$this->processField($subfield, $self))
          return false;
      }
      $rel = TWFormUtil::valueOfFieldRel($field);
      $rev = TWFormUtil::valueOfFieldRev($field);
      if($rel) {
        TWFormUtil::setProperty($this->data, $uri, $rel, $self, "bnode");
      }
      else if($rev) {
        TWFormUtil::setProperty($this->data, $self, $rev, $uri, (0===strpos($uri,"_:") ? "bnode" : "uri"));
      }
      else {
        $this->getLogger()->logStatus("Compound form '$field' not related to parent object by rel nor rev attribute.","error");
        return false;
      }
    }
    return $this->data;
  }

  protected function processPOST() {
    $this->getLogger()->logStatus("TWForm::processPOST");
    $this->data = array();

    // Determine all the fields of the form in the post and what type of
    // field they are. The key of the array is the field name and the
    // value is the type of field. For example, in the class instance
    // form the field "roles" is of type "compound".
    $fields = array();
    foreach($_POST as $field => $value) {
      if(FALSE!==($pos=strrpos($field,"_type"))) {
        $name = substr($field,0,$pos);
        if(FALSE===strpos($name,"_")) {
          $fields[$name] = $_POST[$name."_type"];
        }
      }
    }

    // determine what the URI is of this new instance. If the URI is a
    // file then we grab the file URI. If set by the user then there
    // will be an alias type uri. Else we generate a URI given the type
    // of instance we're creating and time information.
    if(isset($fields["uri"])) {
      $uri = ($fields["uri"] == "file" ? TWFormUtil::valueOfFileField("uri") :
        ($fields["uri"] == "alias" ? TWFormUtil::valueOfAliasField("uri", $this->getBasePath()) :
        $_POST["uri"]));
    } else if( TWForm::$editing && isset($_POST["uri"]) ) {
      $uri = $_POST["uri"];
    } else {
      $uri = TWForm::segmentURI($_POST["class"]);
      $uri = $uri['id'];
      $curTime = gettimeofday();
      $uri .= "_".$curTime["sec"]."_".$curTime["usec"];
      $uri = TWSparql::getEngine()->getIbase().$uri;
    }
    if($uri=="") {
      $this->getLogger()->logStatus("No URI specified in form.","error");
      return null;
    }
    $this->uri = $uri;

    //print( "<br/><br/>uri = $uri" ) ;

    // set the type of the new instance given the class defined in the
    // header
    TWFormUtil::setProperty($this->data, $uri, "http://www.w3.org/1999/02/22-rdf-syntax-ns#type", $_POST["class"], "uri");

    // iterate through each of the fields now and process them given
    // their type.
    foreach($fields as $field => $type) {
      // skip the uri field. We've already dealth with that one
      if($field=="uri") continue;
      // FIXME: we already know the type. It's in the fields array. We
      // are we getting it again?
      $type = $_POST[$field."_type"];
      $this->getLogger()->logStatus("Processing $field of type $type");
      switch($type) {
      case "compound":
        $obj = $this->processCompound($field, $uri);
        if(!$obj) return false;
        break;
      default:
        $obj = $this->processField($field, $uri);
        if(!$obj) return false;
        break;
      }
    }
    return true;
  }

  protected static function getBackupPath() {
    return twforms_get_backup();
  }

  public function checkPOST() {
    $this->getLogger()->logStatus("TWForm::checkPOST");
    if(isset($_POST["twform_id"])) {
      if($this->processPOST()) {
        $ttl = $this->toSPARUL();
        $this->getLogger()->logStatus("Turtle: $ttl");
      }
      if ($ttl) {
        $path = TWForm::getBackupPath();
        if($path!="") {
          $uri = ($_POST["uri_type"]=="file" ? TWFormUtil::valueOfFileField("uri") : $_POST["uri"]);
          $uri = TWForm::segmentURI($_POST["class"]);
          $uri = $uri['id'];
          $curTime = gettimeofday();
          $uri .= "_".$curTime["sec"]."_".$curTime["usec"];
          if(strrpos($path,"/")==strlen($path)-1)
            $path .= "/";
          $fd = fopen($path.$uri.".rdf","w");
          fwrite($fd, $this->toRDF());
          fclose($fd);
          $this->getLogger()->logStatus("Saved backup file.");
        }

        $cmd = "request=".urlencode("INSERT DATA {\r\n".$ttl."\r\n}");
        $log = fopen("/tmp/twforms.log","w");
        fwrite($log,$cmd);
        fclose($log);
        $uri = $this->getUpdateURI();
        if(strpos($uri," ")!==FALSE) {
          $uris = explode(" ",$uri);
          foreach($uris as $uri) {
            $context =
             stream_context_create(array('http' => array("method" => "POST",
             "header" => "Content-type: application/x-www-form-urlencoded\r\n",
             "timeout" => 5,
             "content" => $cmd)));
            file_get_contents($uri,false,$context);
          }
        }
        else {
          $context =
            stream_context_create(array('http' =>
             array("method" => "POST",
             "header" => "Content-type: application/x-www-form-urlencoded\r\n",
             "timeout" => 5,
             "content" => $cmd)));
          $res = file_get_contents($uri,false,$context);
        }
        $uri = twforms_get_rdfsuri();
        $uris = explode(" ",$uri);
        $cmd = "request=".
          urlencode("INSERT DATA { ".
                    "<http://example.com/a> ".
                    "<http://example.com/b> ".
                    "<http://example.com/c> }");
        foreach($uris as $uri) {
          $context =
            stream_context_create(array('http' =>
             array("method" => "POST",
             "header" => "Content-type: application/x-www-form-urlencoded\r\n",
             "timeout" => 5,
             "content" => $cmd)));
          $res = file_get_contents($uri,false,$context);
        }
        $cmd = "request=".
          urlencode("DELETE { ".
          "<http://example.com/a> ?p1 ?o1 . ".
          "<http://example.com/b> ?p2 ?o2 . ".
          "<http://example.com/c> ?p3 ?o3 . ".
          "?s1 <http://example.com/a> ?o4 . ".
          "?s2 <http://example.com/b> ?o5 . ".
          "?s3 <http://example.com/c> ?o6 . ".
          "?s4 ?p4 <http://example.com/a> . ".
          "?s5 ?p5 <http://example.com/b> . ".
          "?s6 ?p6 <http://example.com/c>".
          " } WHERE { ".
          "OPTIONAL { <http://example.com/a> ?p1 ?o1 }".
          "OPTIONAL { <http://example.com/b> ?p2 ?o2 }".
          "OPTIONAL { <http://example.com/c> ?p3 ?o3 }".
          "OPTIONAL { ?s1 <http://example.com/a> ?o4 }".
          "OPTIONAL { ?s2 <http://example.com/b> ?o5 }".
          "OPTIONAL { ?s3 <http://example.com/c> ?o6 }".
          "OPTIONAL { ?s4 ?p4 <http://example.com/a> }".
          "OPTIONAL { ?s5 ?p5 <http://example.com/b> }".
          "OPTIONAL { ?s6 ?p6 <http://example.com/c> }".
        "}");
        foreach($uris as $uri) {
          $context =
            stream_context_create(array('http' => array("method" => "POST",
             "header" => "Content-type: application/x-www-form-urlencoded\r\n",
             "timeout" => 5,
             "content" => $cmd)));
          $res = file_get_contents($uri,false,$context);
        }

        return true;
      }
    }
    return false;
  }

  public function save() {
    if($this->shouldSimulate()) {
      $this->getLogger()->logStatus(htmlspecialchars($this->toSPARUL()));
      return TRUE;
    } else if(isset($_POST["body"]) && $_POST["body"]!="") {
      $title = $_POST["title"];
      $content = base64_decode($_POST["body"]);
      $this->bodyContent = $content;
      $this->getLogger()->logStatus(htmlspecialchars($content));
      return $this->createPage($title, $content);
    }
    return FALSE;
  }

  public abstract function getUpdateURI();
};
