/* -*- espresso-indent-level: 2; -*- */

if(window.xmlns === undefined) window.xmlns = {};

xmlns["tw"] = "http://tw.rpi.edu/schema/";
xmlns["foaf"] = "http://xmlns.com/foaf/0.1/";
xmlns["dc"] = "http://purl.org/dc/terms/";
xmlns["rdfs"] = "http://www.w3.org/2000/01/rdf-schema#";

TWForms = {
  messages: {
    required: "{0} is required.",
    invalidXHTML: "{0} does not contain valid XHTML. {1}",
    noURI: "{0} is set to be an external URI, but no URI was provided.",
    noFile: "{0} should be an uploaded file, but no file has been uploaded.",
    specialize: "Consider selecting a more specific value for {0}",
    subcomponentWarn: "Optional field '{0}' is not specified for {1} {2}",
    subcomponentError: "Required field '{0}' is not specified for {1} {2}",
  },
  getMessage: function(key) {
    var msg = TWForms.messages[key];
    if ( msg === undefined) throw "Invalid Message Identifier.";
    var regex = /{\d+}/g;
    var match = null;
    var lastIndex = 0;
    var str = "";
    while ( match = regex.exec( msg ) ) {
      str += msg.substr( lastIndex, match.index - lastIndex );
      var idx = match[0];
      lastIndex = match.index + idx.length;
      idx = parseInt( idx.substr( 1, idx.length - 1 ) );
      str += arguments[idx + 1];
    }
    str += msg.substr( lastIndex );
    return str;
  },
  queryHeader: function() {
    var res = "";
    for(ns in xmlns) {
      res += "PREFIX "+ns+": <"+xmlns[ns]+">\r\n";
    }
    return res;
  },
  individualsForClass: function(cls, callback) {
    var query = TWForms.queryHeader()+
      "SELECT ?uri ?fname ?lname ?name ?label ?title WHERE {\r\n"+
      "?uri a <"+cls+"> .\r\n"+
      "OPTIONAL { ?uri foaf:name ?name }\r\n"+
      "OPTIONAL { ?uri dc:title ?title }\r\n"+
      "OPTIONAL { ?uri rdfs:label ?label }\r\n"+
      "OPTIONAL { ?uri foaf:firstName ?fname .}\r\n"+
      "OPTIONAL { ?uri foaf:givenname ?fname .}\r\n"+
      "OPTIONAL { ?uri foaf:givenName ?fname .}\r\n"+
      "OPTIONAL { ?uri foaf:lastName ?lname .}\r\n"+
      "OPTIONAL { ?uri foaf:surname ?lname .}\r\n"+
      "OPTIONAL { ?uri foaf:familyName ?lname .}\r\n"+
      "OPTIONAL { ?uri foaf:family_name ?lname .}\r\n"+
      "}";
    var success = function(data) {
      var arr = [];
      $(data).find('result').each(function() {
	var label="",uri="";
	var self=this;
	$(this).find("binding").each(function() {
	  if($(this).attr("name")=="uri") {
	    uri = $(this).find("uri").text();
	  }
	  if($(this).attr("name")=="title") {
	    label = $(this).find("literal").text();
	  }
	  if($(this).attr("name")=="fname"&&label=="") {
	    label = $(this).find("literal").text()+" "+
	      $(self).find("binding[name=\"lname\"]").find("literal").text();
	  }
	  if($(this).attr("name")=="name"&&label=="") {
	    label = $(this).find("literal").text();
	  }
	  if($(this).attr("name")=="label"&&label=="") {
	    label = $(this).find("literal").text();
	  }
	});
	if(label!="" && uri!="") {
	  arr.push({"label":label,"value":uri});
	}
      });
      arr.sort(function(a,b) {
	var c=a.label.toLowerCase();
	var d=b.label.toLowerCase();

	if(c<d) return -1;
	if(d<c) return 1;
	return 0;
      });
      callback(arr);
    };
    $.ajax({type: "GET",
	    url: TWForms.endpoint,
	    data: "query="+encodeURIComponent(query),
	    beforeSend: function(xhr) {
	      xhr.setRequestHeader("Accept", "application/sparql-results+xml");
	    },
	    dataType: "xml",
	    error: function(xhr, text, err) {
	      if(xhr.status==200) {
		success(xhr.responseXML);
	      }
	    },
	    success: success
	   });
  },
  classBasedAutocomplete: function(field, cls) {
    TWForms.individualsForClass(cls,function(data) {
      $(field).autocomplete({"source": data});
    });
  },
  classBasedSelect: function(field, cls) {
    TWForms.individualsForClass(cls,function(data) {
      $(field).children().remove();
      var na = document.createElement("option");
      na.setAttribute("value","");
      na.innerHTML="None";
      $(field).append(na);
      for(var i=0;i<data.length;i++) {
	var opt = document.createElement("option");
	opt.setAttribute("value",data[i].value);
	opt.innerHTML=data[i].label;
	$(field).append(opt);
      }
    });
  },
  buildTree: function(cls, subs, supers) {
    var tree={};
    tree[cls] = {};
    for(var i in subs[cls]) {
      if(i==cls) continue;
      var found=false;
      for(var s in supers[i]) {
	if(s!=i&&cls!=s&&subs[cls][s]!==undefined) {
	  found=true;
	  break;
	}
      }
      if(!found) {
	$.extend(tree[cls],TWForms.buildTree(i, subs, supers));
	tree[cls][i]["label"] = subs[cls][i];
      }
    }
    return tree;
  },
  keySort: function(arr) {
    var result = {};
    var keys = [];
    for(var k in arr) {
      keys.push(k);
    }
    keys.sort(function(a,b) { return (a>b)-(a<b); } );
    for(var i=0;i<keys.length;i++) {
      result[keys[i]] = arr[keys[i]];
    }
    return result;
  },
  buildSelect: function(field, tree, depth) {
    tree = TWForms.keySort(tree);
    for(var uri in tree) {
      if(uri=="label") continue;
      var str = "";
      for(var i=0;i<depth*2;i++) {
	str += "-";
      }
      str += tree[uri]["label"];
      var option = document.createElement("option");
      option.appendChild(document.createTextNode(str));
      option.setAttribute("value",uri);
      field.append(option);
      TWForms.buildSelect(field, tree[uri], depth+1);
    }
  },
  subclassBasedSelect: function(field, cls) {
    var query = TWForms.queryHeader()+
      "SELECT ?y ?x ?label WHERE {\r\n"+
      "?x rdfs:subClassOf ?y .\r\n"+
      "FILTER(str(?y)!=\"http://www.w3.org/2000/01/rdf-schema#Resource\")\r\n"+
      "?x rdfs:subClassOf <"+cls+"> .\r\n"+
      //"FILTER(str(?x)!=\"http://xmlns.com/foaf/0.1/Document\")\r\n"+
      "?x rdfs:label ?label .\r\n"+
      "}";
    var success = function(data) {
      var subs = {}, supers = {};
      var clsLabel="";
      $(data).find("result").each(function() {
	var label="",child="",parent="";
	$(this).find("binding").each(function() {
	  if($(this).attr("name")=="x") {
	    child = $(this).find("uri").text();
	  }
	  if($(this).attr("name")=="y") {
	    parent = $(this).find("uri").text();
	  }
	  if($(this).attr("name")=="label") {
	    label = $(this).find("literal").text();
	  }
	});
	if(child==cls) clsLabel = label;
	if(subs[parent]===undefined) {
	  subs[parent] = {};
	}
	subs[parent][child]=label;
	if(supers[child]===undefined) {
	  supers[child] = {};
	}
	supers[child][parent]=true;
      });
      var tree=TWForms.buildTree(cls, subs, supers);
      tree[cls]["label"] = "None";
      TWForms.buildSelect(field, tree, 0);
    };
    $.ajax({type: "GET",
	    url: TWForms.endpoint,
	    data: "query="+encodeURIComponent(query),
	    beforeSend: function(xhr) {
	      xhr.setRequestHeader("Accept", "application/sparql-results+xml");
	    },
	    dataType: "xml",
	    error: function(xhr, text, err) {
	      if(xhr.status==200) {
		success(xhr.responseXML);
	      }
	    },
	    success: success
	   });
  },
  fileUploadRadios: function(radios,internal,external) {
    $(radios[0]).click(function() {
      internal[0].disabled=false;
      external[0].disabled=true;
    });
    $(radios[1]).click(function() {
      internal[0].disabled=true;
      external[0].disabled=false;
    });
  },
  extendCompoundForm: function(form) {
    window.temp = form;
    var li = $("li",form)[0].cloneNode(true);
    $("li",form).last().before(li);
    $(li).find("input").val("");
    $(li).find("input").each(function () {
      var randId = Math.random().toString(36).substr(2);
      this.id = "dp" + randId;
    });
    $('input[name*="_start"]',li).each(function() {
      $(this).removeClass("hasDatepicker");
      $(this).datepicker({dateFormat:"yy-mm-dd"});
    });
    $('input[name*="_end"]',li).each(function() {
      $(this).removeClass("hasDatepicker");
      $(this).datepicker({dateFormat:"yy-mm-dd"});
    });
    $('.select2-container',li).remove();
    $('select',li).select2();
  },
  labelForField: function(field) {
    var label = field.find("span.label").text();
    if ( /\*$/.test(label) ) {
      label = label.substr( 0, label.length - 1 );
    }
    return label;
  },
  validateCompound: function(field) {
    var valid = true;
    var label = TWForms.labelForField( field );
    TWForms.resetValidation(field);

    // if the compound is not required and no subfields are set then the
    // compound is valid
    if ( !field.hasClass("required") ) {
      var isset = false ;
      $("ul.compoundfield li.subfield", field).each(function(i) {
        $("select option:selected", field).each(function() {
          if ( $(this).text() != "None" ) {
            isset = true ;
          }
        });
        $("textarea, input", this).each(function() {
          if ( $(this).val() != "" ) {
            isset = true ;
          }
        });
      });
      if( isset == false ) return true ;
    }

    // handle compound field
    // if a compound has a least one component that is required, then
    // only those components must be set
    // if no component is explicitly required, all are implicitly required.
    // if a compound allows multiple instances, at least one instance must
    // have all of its reuqired components satisfied for the compound to be
    // satisfied. only those instnaces for which all required components
    // are satisfied are considered satisfied.
    var satisfiedInstances = 0;
    var errorSubcomponent = null;
    var errorSubcomponentId = 0;
    var warningSubcomponent = null;
    var warningIndex = 0;
    var indexOfFirstUnsetRequired = false;
    var indexOfLastCompletedEntry = 0;
    $("ul.compoundfield li.subfield", field).each(function(i) {
      $("select[name!=''], textarea[name!=''], input[name!='']", this).each(function() {
        if ( $(this).val() != "" ) {
          indexOfLastCompletedEntry = i;
        }
      });
      var requiredComponents = $(this).find(".required");
      var hasOptional = true;
      if ( requiredComponents.length == 0 ) {
        // no explicitly required component, so all components are required
        requiredComponents = $(this).find(".input");
        hasOptional = false;
      }
      var satisfied = true;
      var isset = false;
      requiredComponents.find("select[name!=''], textarea[name!=''], input[name!='']").each(function(i) {
        if ( satisfied ) {
          satisfied = satisfied && ($(this).val() != "" || $(this).attr("name").endsWith("_end"));
          if ( !satisfied && errorSubcomponent == null ) {
            errorSubcomponent = $(this).attr("name")
              .match(/_([A-Za-z0-9]*)(\[\])?/)[1];
            errorSubcomponentId = i;
          }
          if ( satisfied ) {
            isset = true;
          }
        }
        first = false;
      });
      if ( satisfied && indexOfFirstUnsetRequired === false )
        satisfiedInstances++;
      else if ( !satisfied && indexOfFirstUnsetRequired === false )
        indexOfFirstUnsetRequired = i;
      if ( satisfied && warningSubcomponent == null ) {
        optionalComponents = $(this).find(":not(.required)");
        optionalComponents.find("select[name!=''], textarea[name!=''], input[name!='']").each(function() {
          if ( $(this).val() == "" ) {
            warningIndex = i + 1;
            warningSubcomponent = $(this).attr("name")
              .match(/_([A-Za-z0-9]*)(\[\])?/)[1];
          }
        });
      }
    });
    if ( satisfiedInstances == 0 && errorSubcomponentId == 0 &&
         $("ul.compoundfield li.subfield", field).length == 1 ) {
      valid = false;
      TWForms.showValidationMessage(
        field, "error", TWForms.getMessage( "required", label )
      );
    } else if ( errorSubcomponent != null &&
                (satisfiedInstances - 1) != indexOfLastCompletedEntry ) {
      valid = false;
      TWForms.showValidationMessage(
        field, "error", TWForms.getMessage( "subcomponentError",
                                            errorSubcomponent, label,
                                            satisfiedInstances + 1 )
      );
    } else if ( warningSubcomponent != null ) {
      TWForms.showValidationMessage(
        field, "warning", TWForms.getMessage( "subcomponentWarn",
                                              warningSubcomponent,
                                              label, warningIndex )
      );
    }
    return valid;
  },
  validateDateTimeRange: function(field, required) {
    var valid = true;
    var label = TWforms.labelForField( field );
    TWForms.resetValidation( field );
    // handle date time field
    var subfields = $( "input", field );
    /* TODO(ewpatton): Update this behavior when we extend the required attribute
     * to include start|end validation.
     */
    var start = subfields[0].val(),
      end = subfields[1].val(),
      start_ms = Date.parse( start ),
      end_ms = Date.parse( end );
    if ( start != "" && isNaN( start_ms ) ) {
      valid = false;
    }
    if ( end != "" && isNaN( end_ms ) ) {
      valid = false;
    }
    return valid;
  },
  validateFileUpload: function(field) {
    var valid = true;
    if ( field.hasClass("required") ) {
        var label = TWForms.labelForField( field );
        TWForms.resetValidation(field);
        // handle file upload field
        if ( $("input[value='external']", field)[0].checked ) {
          // external URL
          if ( $("input[type='text']", field).val() == "" ) {
            valid = false;
            TWForms.showValidationMessage(
              $(field), "error", TWForms.getMessage( "noURI", label )
            );
          }
        } else {
          // internal URL
          var value = $("input[type='hidden'][name$='_internal']", field).val();
          if ( value == undefined || value == "" ) {
            valid = false;
            TWForms.showValidationMessage(
              field, "error", TWForms.getMessage( "noFile", label )
            );
          }
        }
    }
    return valid;
  },
  validateSelect: function(field) {
    var valid = true;
    var label = TWForms.labelForField( field );
    TWForms.resetValidation(field);
    // handle dropdown or multiple select field
    if ( field.hasClass("required") &&
         $("select", field).val() == "" ) {
      valid = false;
      TWForms.showValidationMessage(
        field, "error", TWForms.getMessage( "required", label )
      );
    } else if ( $("select option:selected", field).text() == "None" &&
                $("select", field).val() != "" ) {
      // Use case: subtype selection where the default value (None) is
      // really the root of the type hierarchy (e.g. foaf:Document)
      TWForms.showValidationMessage(
        field, "warning", TWForms.getMessage( "specialize", label )
      );
    }
    return valid;
  },
  validateText: function(field) {
    var valid = true;
    var label = TWForms.labelForField( field );
    TWForms.resetValidation(field);
    if ( field.hasClass("required") &&
         $("textarea, input[type='text']", field).val() == "" ) {
      valid = false;
      TWForms.showValidationMessage(
        field, "error", TWForms.getMessage( "required", label )
      );
    }
    if ( valid && $("textarea", field).length > 0 ) {
      // usually textareas are for XHTML, so we validate by parsing the doc
      try {
        var xhtml = "<div>\n" + $("textarea", field).val() + "\n</div>";
        var doc = null;
        if ( window.DOMParser ) {
          parser = new DOMParser();
          doc = parser.parseFromString(xhtml, "text/xml");
        } else {
          doc = new ActiveXObject("Microsoft.XMLDOM");
          doc.async = false;
          doc.loadXML(xhtml);
        }
        if ( $("parsererror", doc).length > 0 ) {
          valid = false;
          var error = $("parsererror", doc).html();
          TWForms.showValidationMessage(
            field, "error", TWForms.getMessage( "invalidXHTML", label, error ),
            true
          );
        }
      } catch(ex) {
        console.log(ex);
        valid = false;
      }
    }
    return valid;
  },
  resetValidation: function(field) {
    if ( field === undefined ) {
      field = $(".field");
    }
    field.removeClass("error").removeClass("warning")
      .find("span.message").remove();
    // clean up trailing <br> added with message (if any).
    while( field.find(":last").is("br") ) {
      field.find(":last").remove();
    }
  },
  showValidationMessage: function(field, severity, reason, br) {
    field.addClass( severity );
    if ( br ) $("<br>").appendTo(field.find("div.wrapper"));
    var span = $("<span>").addClass("message");
    span.html( severity.substr(0, 1).toUpperCase() +
               severity.substr(1) + ": " + reason );
    field.find("div.wrapper").append(span);
  },
  validateField: function(field) {
    if ( $("ul.compoundfield", field).length > 0 ) {
      return TWForms.validateCompound( field );
    } else if ( $("input[type='radio']", field).length > 0 ) {
      return TWForms.validateFileUpload( field );
    } else if ( $("select", field).length > 0 ) {
      return TWForms.validateSelect( field );
    } else if ( $("textarea, input[type='text']", field).length > 0 ) {
      return TWForms.validateText( field );
    } else {
      // ??? we shouldn't get here
      console.warn("Got a field that client validation doesn't understand");
      return false;
    }
  },
  validate: function(form) {
    TWForms.resetValidation();
    var valid = true;
    // Fixes #491
    // replace non-printing chars (omit cr/lf) with empty string
    var regex = /[\u0000-\u0009\u000B\u000C\u000E-\u001F]/g;
    $("input[type='text'], textarea", form).each(function() {
      var text = $(this).val();
      if ( regex.test( text ) ) {
        $(this).val( text.replace( regex, '' ) );
      }
    });
    // TODO(ewpatton): further validation here
    $("form[name='twform'] .field.required").each(function() {
      if ( !TWForms.validateField( $(this), valid ) ) {
        valid = false;
      }
    });
    $(".field textarea").each(function() {
      valid = valid && TWForms.validateField( $(this).parents(".field") );
    });
    return valid;// valid;
  }
};

$(function() {
  $($("form[name='twform'] fieldset").addClass('inactive').
    click(function() {
      $("form[name='twform'] fieldset").addClass('inactive');
      $(this).removeClass('inactive');
    })[0]).
    removeClass('inactive');
  var aliasField = $(".field [value='alias']");
  if ( aliasField.length > 0 ) {
    aliasField = aliasField.parent().find("input[type='text']");
    if ( aliasField.length > 0 ) {
      var autoaliases = $(".field input[autoalias='true']");
      var aliasText = "";
      autoaliases.keyup(function() {
        aliasText = "";
        autoaliases.each(function() {
          var text = $(this).val();
          text = text.replace(/[^A-Za-z0-9:()-]/g, '_').replace(/_+/g, '_')
            .replace(/^_/, '').replace(/_$/, '');
          aliasText += text;
        });
        aliasField.val(aliasText);
      });
    }
  }
  //$("form[name='twform'] .field").find("textarea, select, input")
  $(document).delegate(
    "form[name='twform']",
    "submit",
    function() {
      TWForms.validateField(
        $(this).parents(".field"), true
      );
    });
  $(document).delegate(
    "form[name='twform'] .field textarea[name!=''], "+
    "form[name='twform'] .field input[name!=''], ",
    "change blur keyup",
    function() {
      TWForms.validateField(
        $(this).parents(".field"), true
      );
    });
});
