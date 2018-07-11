<?php

class NewPerson extends NewInstance {
  public function createNew() {
    return parent::generateURI("Person");
  }

  public function toSPARUL() {
    $output = "<$this->uri> <" . TWForm::namespaces["rdf"] . "type> <" . TWForm::namespaces["foaf"] . "> .\n";
    $output .= "<$this->uri> <" . TWForm::namespaces["foaf"] . "name> \"" . $name . "\"^^<http://www.w3.org/2001/XMLSchema#string> .\n";
    $firstSpace = strpos($name, " ");
    if($firstSpace != -1) {
      $output .= "<$this->uri> <" . TWForm::namespaces["foaf"] . "firstName> \"" . substr($name, 0, $firstSpace) . "\"^^<http://www.w3.org/2001/XMLSchema#string> .\n";
      $output .= "<$this->uri> <" . TWForm::namespaces["foaf"] . "lastName> \"" . substr($name, $firstSpace, strlen($name) - $firstSpace) . "\"^^<http://www.w3.org/2001/XMLSchema#string> .\n";
    }
  }

?>

