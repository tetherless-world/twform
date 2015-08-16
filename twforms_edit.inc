<?php

function twforms_get_forms($node) {
  $content = $node->body;
  $matches = array();
  $forms = array();
  if(preg_match('/<sparql[^>]*\s(i|s|uri)=["\']([\S]*)["\']\s[^>]*>/',$content,$matches)) {
    $instURI = $matches[2];
    if($matches[1] == "i") {
      $instURI = TWSparql::rfc2396(TWSparql::getEngine()->getIbase(),$instURI);
    } else if($matches[1] == "s") {
      $instURI = TWSparql::rfc2396(TWSparql::getEngine()->getSBase(),$instURI);
    }
    $query = "SELECT ?type WHERE { <$instURI> a ?type }";
    $endpoint = TWSparql::getEngine()->getEndpoint();
    $endpoint .= "?query=".urlencode($query);
    $ctx = stream_context_create(array('http' => array("method" => "POST",
						       "header" => "Accept: application/sparql-results+json")));
    $result = @file_get_contents($endpoint,false,$ctx);
    $result = json_decode($result);
    $types = $result->results->bindings;
    foreach($types as $type) {
      $uri = $type->type->value;
      $rs=db_query("SELECT {twforms_forms}.nid, {node}.title FROM {twforms_forms} LEFT JOIN {node} ON {twforms_forms}.nid = {node}.nid WHERE type_uri = '%s'",$uri);
      while($row = $rs->fetch_assoc()) {
        $forms[$row["nid"]] = $row["title"];
      }
      $rs->free();
    }
  }
  return $forms;
}

/**
*
*
*/
function twforms_node_is_instance($node) {
  $forms = twforms_get_forms($node);
  return count($forms)>0;
}

/**
*
*
*/
function twforms_edit_access($node) {
  $access = (user_access('edit rdf instances'));
  $access = $access && node_access('update', $node);
  $access = $access && twforms_node_is_instance($node);

  return $access;
}

function twforms_edit_instance($node, $formid) {
  if(isset($formid) && $formid != NULL) {
    drupal_set_message("a");
    return twforms_edit_form($node, node_load($formid));
  }

  $forms = twforms_get_forms($node);

  switch(count($forms)) {
  case 0:
    break;
  case 1:
    drupal_set_message("b");
    $nids = array_keys($forms);
    return twforms_edit_form($node, node_load($nids[0]));
  default:
    $res = t("This instance can be edited using multiple forms, please pick one:")."<br /><ul>";
    foreach($forms as $nid => $title) {
      $res .= "<li><a href='./rdfedit/$nid'>$title</a></li>";
    }
    $res .= "</ul>";
    return $res;
  }
  return "";
}

function twforms_edit_form($node, $form_node) {
  drupal_add_css(drupal_get_path('module','twforms').'/twforms.css');
  drupal_add_js(drupal_get_path('module','twforms').'/form-common.js');
  $str = "Editing ".$node->title." with form ".$form_node->title;
  $str .= htmlspecialchars($form_node->body);
  $str .= htmlspecialchars(print_r($form_node, TRUE));
  TWForm::$editing = TRUE;
  $form = DrupalTWForm::parseForm($form_node->body, $node, $form_node);
  $form = '<div class="node"><div class="node">'.$form.'</div></div>';
  $test = twdocs_process($str.$form->render(),$node);
  return $test;
}
