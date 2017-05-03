<?php

class NewOrganization extends NewInstance {
  public function createNew() {
    return parent::generateURI("Organization");
  }

  public function toSPARUL() {
  }

?>

