<?php

include_once("TWFormUnitTest.inc");

global $formBody;
global $errors;
$errors=FALSE;

$formBody = <<<EOF
<twformheader class="foaf:Person" type="person" xmlns:foaf="http://xmlns.com/foaf/0.1/" xmlns:tw="http://tw.rpi.edu/schema/" xmlns:dc="http://purl.org/dc/terms/"  xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#" />

<twformbody>
<sparql query="person.rq" uri="$uri" xslt="generate/person-header.xsl" />
<sparql xslt="generate/person-interests.xsl" />
<sparql xslt="generate/person-bio.xsl" />
<sparql xslt="generate/person-affiliates.xsl" />
<sparql xslt="generate/author-publications.xsl" />
<sparql xslt="generate/author-presentations.xsl" />
<sparql xslt="generate/person-projects.xsl" />
</twformbody>

<fieldset>
<legend>Basic Information</legend>
<div>
<twformfield id="uri" title="URI" type="alias" base="http://tw.rpi.edu/instances" required="true" />
<twformfield id="person_title" title="Title (e.g. Mr., Mrs., Dr.)" type="text" rel="foaf:title" />
<twformfield id="firstName" title="First Name" required="true" type="text" rel="foaf:firstName" />
<twformfield id="middle" title="Middle Initial" type="text" rel="tw:middleInitial" />
<twformfield id="lastName" title="Last Name" required="true" type="text" rel="foaf:lastName" />
<twformfield id="suffix" title="Suffix (e.g. Ph.D., B.Sc, Jr.)" type="text" rel="tw:nameSuffix" />
<twformfield id="title" title="Display Name" required="true" type="text" rel="foaf:name" />
<twformfield id="alias" title="Local URL" type="alias" base="person" rel="foaf:page" required="true" />
</div>
</fieldset>

<fieldset>
<legend>Profile</legend>
<div>
<twformfield id="depiction" title="Depiction" type="file" rel="foaf:depiction" />
<twformfield id="bio" title="Bio" required="true" type="textarea" rel="tw:hasBio" />
</div>
</fieldset>

<fieldset>
<legend>On The Web</legend>
<div>
<twformfield id="email" title="Email" type="email" rel="foaf:mbox" />
<twformfield id="weblog" title="Blog" type="url" rel="foaf:weblog" />
<twformfield id="homepage" title="Homepage" type="url" rel="foaf:homepage" />
<twformfield id="school_homepage" title="School Homepage" type="url" rel="foaf:schoolHomepage" />
<twformfield id="work_homepage" title="Workplace Homepage" type="url" rel="foaf:workplaceHomepage" />
<twformfield id="seeAlso" title="See Also" type="url" rel="rdfs:seeAlso" />
</div>
</fieldset>

<fieldset>
<legend>Roles</legend>
<div>
<twformcompound id="role" title="Roles" more="Add additional roles" rel="tw:hasRole" typeof="tw:OrganizationalRole"><twformfield id="theyare" title="Type" type="subclass" />
 at <twformfield id="affiliation" type="instance" class="foaf:Organization" rel="tw:withAffiliation" />
 between <twformfield id="date" type="daterange" rel="tw:hasDateTimeCoverage" wrap="false" />
</twformcompound>
</div>
</fieldset>

<fieldset>
<legend>Permissions</legend>
<div>
<twformcompound id="account" title="LDAP Account" rel="foaf:account" typeof="foaf:OnlineAccount"><twformfield id="ldap" type="text" rel="foaf:accountName" /></twformcompound><span class="info">Required if drupal account is created and edit permissions given</span>
</div>
</fieldset>

<twformsubmit title="Create Person Instance"/>
EOF;

class TWFormTest extends TWFormUnitTest {
  
  public function test() {
    global $formBody,$errors;
    $this->getLogger()->logStatus("Parsing form");
    $this->parse($formBody);
    if($errors) return;
    if(TWForm::$editing) {
      $this->getLogger()->logStatus("Populating form");
      $this->prepopulate($this->getInstanceUri());
    }
  }
  
};

TWForm::$editing = TRUE;
$form = new TWFormTest();
$form->test();
if($errors) {
  echo "FAILED\n";
}
else {
  echo "SUCCESS\n";
}
