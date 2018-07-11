<?php

class NewConcept extends NewInstance {
  public function createNew() {
    return parent::generateURI("Concept");
  }

  public function toSPARUL() {
  }

?>

