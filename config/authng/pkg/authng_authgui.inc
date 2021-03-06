<?php
/* $Id$ */
/* ========================================================================== */
/*
    authng_authgui.inc
    part of pfSense (http://www.pfSense.com)
    Copyright (C) 2007 Daniel S. Haischt <me@daniel.stefan.haischt.name>
    All rights reserved.

    Based on m0n0wall (http://m0n0.ch/wall)
    Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
    All rights reserved.
                                                                              */
/* ========================================================================== */
/*
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

     1. Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
    AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
    OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
                                                                              */
/* ========================================================================== */

require_once("authng.inc");

/* Authenticate user - exit if failed (we should have a callback for this maybe) */
if (empty($authMethod)) { print "auth_method missing!\n"; }
if (empty($backend)) { print "backing_method missing!\n"; }
if (!$authMethod->authenticate($backend)) { exit; }

/* scriptname is set in headjs.php if the user did try to access a page other
 * than index.php without beeing logged in.
 */
if (isset($_POST['scriptname']) && $userPeer->isSystemAdmin($HTTP_SERVER_VARS['AUTH_USER'])) {
  pfSenseHeader("{$_POST['scriptname']}");
  exit;
}

$allowed = array();

// Once here, the user has authenticated with the web server.
// Now, we give them access only to the appropriate pages for their group.
if (!($userPeer->isSystemAdmin($HTTP_SERVER_VARS['AUTH_USER']))) {
  $allowed[] = '';
  if (isset($config['system']['group'][$groupindex[$config['system']['user'][$userindex[$HTTP_SERVER_VARS['AUTH_USER']]]['groupname']]]['pages'][0]['page'])) {
    $useridx = $userindex[$HTTP_SERVER_VARS['AUTH_USER']];
    $grouidx = $groupindex[$config['system']['user'][$useridx]];
    $allowed = &$config['system']['group'][$groupidx]['pages'][0]['page'];
  }

  $group = $config['system']['user'][$userindex[$HTTP_SERVER_VARS['AUTH_USER']]]['groupname'];
  /* get the group homepage, to be able to forward
   * the user to this particular PHP page.
   */
  $groupPeer->getGroupHomePage($group) == "" ? $home = "/index.php" : $home = "/" . $groupPeer->getGroupHomePage($group);

  /* okay but if the user realy tries to explicitely access a particular
   * page, set $home to that page instead.
   */
  if (isset($_POST['scriptname']) && $_POST['scriptname'] <> "/" && $_POST['scriptname'] <> "/index.php")
    $home = basename($_POST['scriptname']);

  // If the user is attempting to hit the default page, set it to specifically look for /index.php.
  // Without this, any user would have access to the index page.
  //if ($_SERVER['SCRIPT_NAME'] == '/')
  //  $_SERVER['SCRIPT_NAME'] = $home;

  // Strip the leading / from the currently requested PHP page
  if (!in_array(basename($_SERVER['SCRIPT_NAME']),$allowed)) {
    // The currently logged in user is not allowed to access the page
    // they are attempting to go to.  Redirect them to an allowed page.

    if(stristr($_SERVER['SCRIPT_NAME'],"sajax")) {
      echo "||Access to AJAX has been disallowed for this user.";
      exit;
    }

    if ($home <> "" && in_array($home, $allowed)) {
      pfSenseHeader("{$home}");
      exit;
    } else {
      header("HTTP/1.0 401 Unauthorized");
      header("Status: 401 Unauthorized");

      echo display_error_form("401", "401 Unauthorized. Authorization required.");
      exit;
    }
  }

  if (isset($_SESSION['Logged_In'])) {
    /*
     * only forward if the user has just logged in
     * TODO: session auth based - may be an issue.
     */
    if ($_SERVER['SCRIPT_NAME'] <> $home && empty($_SESSION['First_Visit'])) {
      $_SESSION['First_Visit'] = "False";
      pfSenseHeader("{$home}");
      exit;
    }
  }
}

function display_error_form($http_code, $desc) {
  global $g;

  $htmlstr = <<<EOD
<html>
  <head>
    <script type="text/javascript" src="/javascript/scriptaculous/prototype.js"></script>
    <script type="text/javascript" src="/javascript/scriptaculous/scriptaculous.js"></script>
    <title>An error occurred: {$http_code}</title>
    <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
    <link rel="shortcut icon" href="/themes/{$g['theme']}/images/icons/favicon.ico" />
    <link rel="stylesheet" type="text/css" href="/themes/{$g['theme']}/all.css" media="all" />
    <style type="text/css">
    #errordesc {
      background: #cccccc;
      border: 0px solid #666666;
      margin: 5em auto;
      padding: 0em;
      width: 340px;
    }
    #errordesc h1 {
      background: url(/themes/{$g['theme']}/images/misc/logon.png) no-repeat top left;
      margin-top: 0;
      display: block;
      text-indent: -1000px;
      height: 50px;
      border-bottom: none;
    }

    #login p {
      font-size: 1em;
      font-weight: bold;
      padding: 3px;
      margin: 0em;
      text-indent: 10px;
    }

    #login span {
      font-size: 1em;
      font-weight: bold;
      width: 20%;
      padding: 3px;
      margin: 0em;
      text-indent: 10px;
    }

    #login p#text {
      font-size: 1em;
      font-weight: normal;
      padding: 3px;
      margin: 0em;
      text-indent: 10px;
    }
    </style>

    <script type="text/javascript">
    <!--
      function page_load() {
        NiftyCheck();
        Rounded("div#errordesc","bl br","#333","#cccccc","smooth");
        Effect.Pulsate('errortext', { duration: 10 });
      }
      <?php
        require("headjs.php");
        echo getHeadJS();
      ?>
    //-->
    </script>
    <script type="text/javascript" src="/themes/{$g['theme']}/javascript/niftyjsCode.js"></script>
  </head>
  <body onload="page_load();">
    <div id="errordesc">
      <h1>&nbsp</h1>
      <p id="errortext" style="vertical-align: middle; text-align: center;"><span style="color: #000000; font-weight: bold;">{$desc}</span></p>
    </div>
  </body>
</html>

EOD;

  return $htmlstr;
}

function display_login_form() {
  require_once("globals.inc");
  global $g;

  if(isAjax()) {
    if (isset($_POST['login'])) {
      if($_SESSION['Logged_In'] <> "True") {
        isset($_SESSION['Login_Error']) ? $login_error = $_SESSION['Login_Error'] : $login_error = "unknown reason";
        echo "showajaxmessage('Invalid login ({$login_error}).');";
      }
      if (file_exists("{$g['tmp_path']}/webconfigurator.lock")) {
        $whom = file_get_contents("{$g['tmp_path']}/webconfigurator.lock");
        echo "showajaxmessage('This device is currently beeing maintained by: {$whom}.');";
      }
    }
    exit;
  }

?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
  <head>
    <script type="text/javascript" src="/javascript/scriptaculous/prototype.js"></script>
    <script type="text/javascript" src="/javascript/scriptaculous/scriptaculous.js"></script>
    <title><?=gettext("Login"); ?></title>
    <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
    <link rel="shortcut icon" href="/themes/<?= $g['theme'] ?>/images/icons/favicon.ico" />
    <?php if (file_exists("{$g['www_path']}/themes/{$g['theme']}/login.css")): ?>
    <link rel="stylesheet" type="text/css" href="/themes/<?= $g['theme'] ?>/login.css" media="all" />
    <?php else: ?>
    <link rel="stylesheet" type="text/css" href="/themes/<?= $g['theme'] ?>/all.css" media="all" />
    <?php endif; ?>
    <script type="text/javascript">
    <!--
      <?php if (file_exists("{$g['www_path']}/themes/{$g['theme']}/login.css")): ?>
      var dontUseCustomBGColor = false;
      <?php else: ?>
      var dontUseCustomBGColor = true;
      <?php endif; ?>
      function page_load() {
        NiftyCheck();
        Rounded("div#login","bl br","#333","#cccccc","smooth");
        document.login_iform.usernamefld.focus();
      }
      function clearError() {
        if($('inputerrors'))
          $('inputerrors').innerHTML='';
      }
      <?php
//        require("headjs.php");
//        echo getHeadJS();
      ?>
    //-->
    </script>
    <script type="text/javascript" src="/themes/<?= $g['theme'] ?>/javascript/niftyjsCode.js"></script>
  </head>
  <body onload="page_load()">
    <div id="login">
      <h1>&nbsp;</h1>
      <form id="iform" name="login_iform" method="post" autocomplete="off" action="<?= $_SERVER['SCRIPT_NAME'] ?>">
        <div id="inputerrors"></div>
        <p>
          <span style="text-align: left;">
            <?=gettext("Username"); ?>:&nbsp;&nbsp;
          </span>
          <input onclick="clearError();" onchange="clearError();" id="usernamefld" type="text" name="usernamefld" class="formfld user" tabindex="1" />
        </p>
        <p>
          <span style="text-align: left;">
            <?=gettext("Password"); ?>:&nbsp;&nbsp;
          </span>
          <input onclick="clearError();" onchange="clearError();" id="passwordfld" type="password" name="passwordfld" class="formfld pwd" tabindex="2" />
        </p>
        <table width="90%" style="margin-right: auto; margin-left: auto;">
          <tr>
            <td valign="middle" align="right" style="font-style: italic;"><br /><?=gettext("Enter username and password to login."); ?></td>
            <td valign="middle" align="left"><input type="submit" id="submit" name="login" class="formbtn" value="<?=gettext("Login"); ?>" tabindex="3" /></td>
          </tr>
        </table>
      </form>
    </div>
  </body>
</html>
<?php
} // end function
?>