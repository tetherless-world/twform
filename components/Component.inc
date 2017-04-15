<?php

abstract class Component {
  protected $type;
  protected $id;
  protected $raw_id;
  protected $title=null;
  protected $required=false;
  protected $autoalias=false;
  protected $editing=false;
  protected $namespaces;
  protected $rel=null;
  protected $rev=null;
  protected $instClass=null;
  protected $isArray=false;
  protected $results=null;
  private static $varnum = 0;
  private $sparqlVar=null;
  protected $logger = null;
  protected $form = null;

  public function getVar($i = 0) {
    if($this->sparqlVar == null) $this->sparqlVar = array();
    if(!isset($this->sparqlVar[$i]))
      $this->sparqlVar[$i] = "?v".self::$varnum++;
    return $this->sparqlVar[$i];
  }

  public function getBlank() {
    return "?v".self::$varnum++;
  }

  abstract public function init($space);
  abstract public function render();
  abstract public function prepopulate($inst, $top);
  abstract public function postpopulate($inst, $entries);
  abstract public function getValue();
  abstract public function toSPARUL($node);

  /**
   * Validates a component in the form.
   * @return array An array of errors when the component validated, will be
   * empty if successful.
   */
  public function validate() {
    return array("Field ".$this->getId()." is not valid.");
  }

  public function getForm() { return $this->form; }
  public function setForm($form) { $this->form = $form; }

  public function getType() { return $this->type; }
  public function getId() { return $this->id; }
  public function getTitle() { return $this->title; }
  public function isRequired() { return $this->required; }
  public function getNamespaces() { return $this->namespaces; }
  public function getRel() { return $this->rel; }
  public function getRev() { return $this->rev; }

  public function getIsArray() { return $this->isArray; }
  public function setIsArray($bool) { $this->isArray=$bool; }

  public function getEditing() { return $this->editing; }
  public function setEditing($bool) { $this->editing=$bool; }

  public function getInstClass() { return $this->instClass; }
  public function setInstClass($cls) { $this->instClass=$cls; }

  public function getResults() { return $this->results; }
  public function setResults($r) { $this->results=$r; }

  public function setNamespaces(&$ns) {
    $this->namespaces = $ns;
  }

  public function setLogger($l) {
    $this->logger = $l;
  }

  public function getLogger() {
    return $this->logger;
  }

  protected function queryHeader() {
    return TWForm::getQueryHeader();
  }

  protected function individualsForClass($cls) {
    $query = $this->queryHeader();
    $query .=
      "SELECT ?uri ?name ?title ?fname ?lname ?label WHERE {\r\n".
      "?uri a <$cls> .\r\n".
      "OPTIONAL { ?uri foaf:name ?name . FILTER(lang(?name)=\"\" || lang(?name)=\"en\") }\r\n".
      "OPTIONAL { ?uri dc:title ?title }\r\n".
      "OPTIONAL { ?uri rdfs:label ?label }\r\n".
      "OPTIONAL { ?uri skos:prefLabel ?label }\r\n".
      "OPTIONAL { ?uri foaf:firstName ?fname . FILTER(lang(?fname)=\"\" || lang(?fname)=\"en\") }\r\n".
      "OPTIONAL { ?uri foaf:givenname ?fname . FILTER(lang(?fname)=\"\" || lang(?fname)=\"en\") }\r\n".
      "OPTIONAL { ?uri foaf:givenName ?fname . FILTER(lang(?fname)=\"\" || lang(?fname)=\"en\") }\r\n".
      "OPTIONAL { ?uri foaf:lastName ?lname . FILTER(lang(?lname)=\"\" || lang(?lname)=\"en\") }\r\n".
      "OPTIONAL { ?uri foaf:surname ?lname . FILTER(lang(?lname)=\"\" || lang(?lname)=\"en\") }\r\n".
      "OPTIONAL { ?uri foaf:familyName ?lname . FILTER(lang(?lname)=\"\" || lang(?lname)=\"en\") }\r\n".
      "OPTIONAL { ?uri foaf:family_name ?lname . FILTER(lang(?lname)=\"\" || lang(?lname)=\"en\") }\r\n".
      "FILTER(!isBlank(?uri))".
      "}";
    $params = array();
    $engine = TWSparql::getEngine();
    $error = NULL;
    $rdf = $engine->executeQueryDOM( $engine->getEndpoint(), $query, $error );
    if($error) {
      drupal_set_message($error);
      return array();
    }
    $results = $rdf->documentElement->getElementsByTagName("result");
    $rLen = $results->length;
    $ans = array();
    for($i=0;$i<$rLen;$i++) {
      $uri = null; $label=null;
      $result = $results->item($i);
      $bindings = $result->getElementsByTagName("binding");
      $bLen = $bindings->length;
      for($j=0;$j<$bLen;$j++) {
	$binding = $bindings->item($j);
	$name = $binding->getAttribute("name");
	switch($name) {
	case "uri":
	  $uri = trim($binding->textContent);
	  break;
	case "title":
	  $label = trim($binding->textContent);
	  break;
	case "name":
	  $label = trim($binding->textContent);
	  break;
	case "fname":
	  $label = trim($binding->textContent);
	  for($k=0;$k<$bLen;$k++) {
	    $binding2 = $bindings->item($k);
	    $name2 = $binding2->getAttribute("name");
	    if($name2=="lname") {
	      $label .= " ".trim($binding2->textContent);
	      break;
	    }
	  }
	  break;
	case "label":
	  $label = trim($binding->textContent);
	  break;
	}
	if($uri != null && $label != null) break;
      }
      if($label != "" && $uri != "")
	$ans[$uri] = $label;
    }
    asort($ans);
    return $ans;
  }

  protected function extend(&$a, $b) {
    foreach($b as $prop=>$val) {
      $a[$prop] = $val;
    }
  }

  protected function buildTree($cls, &$subs, &$supers) {
    $tree = array();
    $tree[$cls] = array();
    foreach($subs[$cls] as $uri=>$data) {
      if($uri == $cls) continue;
      $found=false;
      foreach($supers[$uri] as $s=>$data2) {
	if($s!=$uri && $cls != $s && isset($subs[$cls][$s])) {
	  $found = true;
	  break;
	}
      }
      if(!$found) {
	$this->extend($tree[$cls], $this->buildTree($uri, $subs, $supers));
	$tree[$cls][$uri]["label"] = $subs[$cls][$uri];
      }
    }
    return $tree;
  }

  protected function buildSelect($tree, $depth) {
    ksort($tree);
    $res = "";
    foreach($tree as $uri=>$data) {
      if($uri == "label") continue;
      $str = "";
      for($i=0;$i<$depth;$i++) {
	$str .= "--";
      }
      $str .= $data["label"];
      $res .= "<option value=\"$uri\"";
      if(is_array($this->values) && in_array($uri,$this->values)) {
	$res .= ' selected="selected" ';
      }
      $res .= ">$str</option>";
      $res .= $this->buildSelect($data, $depth+1);
    }
    return $res;
  }

  protected function subclassBasedSelect($cls) {
    $subs = array(); $supers = array();
    $clsLabel = "";
    $query = $this->queryHeader().
      "SELECT ?y ?x ?label WHERE {\r\n".
      "?x rdfs:subClassOf ?y .\r\n".
      "FILTER(str(?y)!=\"http://www.w3.org/2000/01/rdf-schema#Resource\")\r\n".
      "?x rdfs:subClassOf <$cls> .\r\n".
      "?x rdfs:label ?label .\r\n".
      "}";
    $params = array();
    $engine = TWSparql::getEngine();
    $error = NULL;
    $rdf = $engine->executeQueryDOM( $engine->getEndpoint(), $query, $error );
    if($error) {
      drupal_set_message($error);
      return "";
    }
    $results = $rdf->getElementsByTagName("result");
    $rLen = $results->length;
    for($i=0;$i<$rLen;$i++) {
      $result = $results->item($i);
      $label = ""; $child=""; $parent="";
      $bindings = $result->getElementsByTagName("binding");
      $bLen = $bindings->length;
      for($j=0;$j<$bLen;$j++) {
	$binding = $bindings->item($j);
	$name = $binding->getAttribute("name");
	switch($name) {
	case "x":
	  $child = trim($binding->textContent);
	  break;
	case "y":
	  $parent = trim($binding->textContent);
	  break;
	case "label":
	  $label = trim($binding->textContent);
	  break;
	}
      }
      if($child==$cls) $clsLabel = $label;
      if(!isset($subs[$parent])) {
	$subs[$parent] = array();
      }
      $subs[$parent][$child] = $label;
      if(!isset($supers[$child])) {
	$supers[$child] = array();
      }
      $supers[$child][$parent]=true;
    }
    $tree = $this->buildTree($cls,$subs,$supers);
    $tree[$cls]["label"] = "None";
    return $this->buildSelect($tree, 0);
  }

  protected function resolve($uri) {
    if(0===strpos($uri,"http://")) return $uri;
    $parts = explode(":",$uri);
    if(count($parts)!=2) {
      drupal_set_message("Invalid CURIE $uri.","error");
      return $uri;
    }
    //return TWForm::expandCURIE($uri);
    if(!isset(TWForm::$namespaces[$parts[0]])) {
      drupal_set_message("Undeclared namespace $parts[0]","error");
      return $uri;
    }
    return TWForm::$namespaces[$parts[0]].$parts[1];
  }

  public function processParams(&$params) {
    $this->id = $params["id"];
    $this->raw_id = $params["id"];
    if(isset($params["title"]))
      $this->title = $params["title"];
    if(isset($params["required"]))
      $this->required = $params["required"];
    if(isset($params["rel"]))
      $this->rel = $this->resolve($params["rel"]);
    if(isset($params["rev"]))
      $this->rev = $this->resolve($params["rev"]);
    if(isset($params["editing"]))
      $this->editing = $params["editing"];
    if(isset($params["autoalias"]))
      $this->autoalias = $params["autoalias"];
    if($this->editing) {}
      //implement function to build and execute SPARQL query for component
      //$this->results = twforms_get_instance_data($params);
    return true;
  }

  public function render_structure() {
    $type = '<input type="hidden" name="'.$this->id.'_type" value="'.$this->type.'"/>';
    if($this->rel!=null) {
      return '<input type="hidden" name="'.$this->id.'_rel" value="'.$this->rel.'"/>'.$type;
    }
    else if($this->rev!=null) {
      return '<input type="hidden" name="'.$this->id.'_rev" value="'.$this->rev.'"/>'.$type;
    }
    else {
      return $type;
    }
  }

  public function setPrefix($prefix) {
    $this->id = $prefix.$this->raw_id;
  }

  public function dump($spaces)
  {
      $this->getLogger()->logStatus( "$spaces Component:" ) ;
      $newspaces = $spaces . "&nbsp;&nbsp;&nbsp;&nbsp;" ;

      $this->getLogger()->logStatus( "$newspaces type = $this->type" ) ;
      $this->getLogger()->logStatus( "$newspaces id = $this->id" ) ;
      $this->getLogger()->logStatus( "$newspaces raw_id = $this->raw_id" ) ;
      if( $this->title != null ) $this->getLogger()->logStatus( "$newspaces title = $this->title" ) ;

      if( $this->required ) $this->getLogger()->logStatus( "$newspaces required = true" ) ;
      else $this->getLogger()->logStatus( "$newspaces required = false" ) ;

      if( $this->autoalias ) $this->getLogger()->logStatus( "$newspaces autalias = true" ) ;
      else $this->getLogger()->logStatus( "$newspaces autalias = false" ) ;

      if( $this->editing ) $this->getLogger()->logStatus( "$newspaces editing = true" ) ;
      else $this->getLogger()->logStatus( "$newspaces editing = false" ) ;

      if( $this->rel != null ) $this->getLogger()->logStatus( "$newspaces rel = $this->rel" ) ;
      if( $this->rev != null ) $this->getLogger()->logStatus( "$newspaces rev = $this->rev" ) ;
  }
};
