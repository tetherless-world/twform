 <?php

require_once("components/TWForm.inc");

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

class DrupalTWFormLogger extends PHPTWFormLogger {
  public function logError($str) {
    drupal_set_message($str, 'error');
  }

  public function logWarning($str) {
    drupal_set_message($str, 'warning');
  }

  public function logStatus($str) {
    drupal_set_message($str, 'status');
  }
}

class DrupalTWForm extends TWForm {

  protected $node = NULL;
  protected $form = NULL;

  protected function getAction() {
    return base_path().(isset($this->node->alias) ? $this->node->alias : "node/".$this->node->nid);
  }

  protected function getInstanceUri() {
    $content = $this->node->body;
    $matches = array();
    preg_match('/<sparql[^>]*\s(i|s|uri)=["\']([\S]*)["\']\s[^>]*>/',$content,$matches);
    $instURI = $matches[2];
    if($matches[1] == "i") {
      $instURI = TWSparql::rfc2396(TWSparql::getEngine()->getIbase(), $instURI);
    } else if($matches[1] == "s") {
      $instURI = TWSparql::rfc2396(TWSparql::getEngine()->getSbase(), $instURI);
    }
    return $instURI;
  }

  protected function getLogger() {
    if(TWForm::$logger == NULL) {
      TWForm::$logger = new DrupalTWFormLogger();
    }
    return TWForm::$logger;
  }

  public static function init() {
    TWForm::$logger = new DrupalTWFormLogger();
  }

  public static function parseForm($content, &$node, &$form_node = NULL) {
    $form = new DrupalTWForm();
    $form->node = &$node;
    if(!TWForm::$editing) {
      $form->form = &$node;
    } else {
      $form->form = &$form_node;
    }
    $form->parse($content);
    if(TWForm::$editing) {
      $form->prepopulate($form->getInstanceUri());
    }
    return $form;
  }

  protected function getBasePath() {
    return base_path();
  }

  protected function shouldSimulate() {
    return variable_get('twforms_simulate',FALSE);
  }

  protected function getId() {
    return $this->form->nid;
  }

  public function createPage($title, $content) {
    global $user;
    drupal_set_message("DrupalTWForm::createPage");
    module_load_include("inc","node","node.pages");
    if(isset($this->uri))
      $uri = $this->uri;
    else if ($_POST['uri_type'] == 'file')
      $uri = $this->valueOfFileField('uri');
    else if ($_POST['uri_type'] == 'alias')
      $uri = $this->valueOfAliasField('uri');
    else $uri = $_POST['uri'];
    if (isset($_POST["contenttype"]))
      $type = $_POST["contenttype"];
    else
      $type = "page";
    $content = preg_replace('/\\$uri/', $uri, $this->bodyContent);
    $node = array("type" => $type);
    $this->getLogger()->logStatus("twforms_create_page: type = '$type'");
    $form_state = array();
    $form_state['values'] = array();
    $mytitle = $_POST['title'];
    $form_state['values']['title'] = $mytitle;
    $this->getLogger()->logStatus("twforms_create_page: title = '$mytitle'");
    $form_state['values']['body'] = $content;
    $this->getLogger()->logStatus("twforms_create_page: content = '$content'");
    $form_state['values']['uid'] = $user->uid;
    $this->getLogger()->logStatus("twforms_create_page: user id = '$user->uid'");
    $form_state['values']['name'] = $user->name;
    $this->getLogger()->logStatus("twforms_create_page: user name = '$user->name'");
    $form_state['values']['status'] = 1;
    $form_state['values']['format'] = 2;

    // if an alias is specified, then we set the URL path settings to that
    if (isset($_POST['alias'])) {
      $mypath = $_POST["alias_base"] . '/' . $_POST["alias"];
      $form_state['values']['path'] = $mypath ;
      $this->getLogger()->logStatus("twforms_create_page: path = '$mypath'");
    }
    else {
      $this->getLogger()->logStatus("twforms_create_page: path = NONE SET");
    }
    $form_state['values']['op'] = t("Save");
    drupal_execute($type."_node_form", $form_state, (object)$node);
    $res = form_get_errors();
    if($res && count($res)>0) {
      $this->getLogger()->logStatus(htmlspecialchars(print_r($res, true)));
      return FALSE;
    }

    // redirect to the node/number page, not the local alias specified
    $myredirect = "Location: ".base_path()."node/".$form_state["nid"];
    header($myredirect);
    return TRUE;
  }

  public function getAltBase() {
    return twforms_get_alt_base();
  }

  public function getUpdateURI() {
    return twforms_get_updateuri();
  }
}
