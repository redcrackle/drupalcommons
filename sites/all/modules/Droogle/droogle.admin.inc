<?php

/**
 * @file
 * Administration function for droogle.
 */

/**
 * Uses the form api for an admin settings form.
 */
function droogle_settings_form() {
  global $user;
  $droogle_clientid = variable_get('droogle_clientid', '');
  $droogle_secret = variable_get('droogle_secret', '');
  $droogle_title_text = variable_get('droogle_title_text', '');
  $droogle_password = variable_get('droogle_password', '');
  $droogle_refresh_token = variable_get('droogle_refresh_token', '');
  $droogle_og_clientid_field = variable_get('droogle_og_field_gdrive_client_id');
  $droogle_og_clientsecret_field = variable_get('droogle_og_field_gdrive_client_secret');
  $droogle_og_field_gdrive_refresh_token = variable_get('droogle_og_field_gdrive_refresh_token');

  $form['sitewide'] = array(
    '#type' => 'fieldset',
    '#title' => t('Sitewide Droogle settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('To create client IDs, secret: <ol><li>Visit <a href="@url">Google Console</a> and create a project to use.</li> <li>Enable the Drive API and the Drive SDK under APIs tab</li><li>Generate client IDs/Secret under the Credentials tab</li><li>Add @url_refresh to "Redirect URIs" for the new client ID.</ol>', array(
      '@url' => 'https://cloud.google.com/console',
      '@url_refresh' => "http://" . $_SERVER['HTTP_HOST'] . "/droogle_get_google_token")),
  );

  $form['sitewide']['droogle_title_text'] = array(
    '#type' => 'textfield',
    '#title' => t('Droogle Page Title'),
    '#default_value' => $droogle_title_text,
    '#size' => 60,
    '#maxlength' => 64,
    '#description' => t('Enter the title to put at the top of the <a href="@url">droogle page</a> (when not within an Organic Groups context), default is:  "DROOGLE: A list of your google docs"', array('@url' => url('droogle'))),
  );

  $form['sitewide']['droogle_clientid'] = array(
    '#type' => 'textfield',
    '#title' => t('Client ID'),
    '#default_value' => $droogle_clientid,
    '#size' => 100,
    '#maxlength' => 150,
    '#description' => t('The site wide google client id.'),
  );

  $form['sitewide']['droogle_secret'] = array(
    '#type' => 'textfield',
    '#title' => t('Client secret'),
    '#default_value' => $droogle_secret,
    '#description' => t('The site wide google client secret.'),
  );

  $refresh_token_label = t('Refresh Token');
  $form['sitewide']['droogle_refresh_token'] = array(
    '#type' => 'textfield',
    '#title' => t('Refresh Token'),
    '#description' => t('To generate a <a href="@google_url">refresh token</a> (which is necessary for secure communication), configure client ID and secret, then press "@refresh_token_label" below', array('@google_url' => 'https://developers.google.com/identity/protocols/OAuth2WebServer#offline', '@refresh_token_label' => $refresh_token_label)),
    '#default_value' => $droogle_refresh_token,
  );

  if (module_exists('og')) {
    $text_fields = array('' => t('None'));

    foreach (field_info_fields() as $field_name => $field) {
      // This restricts it to just text fields or select, but if someone wants
      // to add addtional widgets, they can set this variable to new values.
      if (in_array($field['module'], variable_get('droogle_text_fields_type', array('text', 'list_text')))) {
        if (($entity_type = key($field['bundles'])) && ($bundle = reset($field['bundles'][$entity_type])) && ($instance = field_info_instance($entity_type, $field_name, $bundle)))
        $text_fields[$field_name] = t('@label (@field_name)', array('@label' => $instance['label'], '@field_name' => $field_name));
      }
    }

    $form['single_groups'] = array(
      '#type' => 'fieldset',
      '#title' => t('Organic Groups configuration'),
      '#description' => t('To allow organic groups to override site wide settings, add two text field (textfield or select) that will allow users to input client ID and secret on the add/edit form for the group. When used, this will override site wide settings.'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );

    $form['single_groups']['droogle_og_field_gdrive_client_id'] = array(
      '#type' => 'select',
      '#options' => $text_fields,
      '#title' => t('Client ID field'),
      '#default_value' => $droogle_og_clientid_field,
      '#description' => t('Select the text field on a group content type that will contain the overriding client ID for that organic group.'),
    );

    $form['single_groups']['droogle_og_field_gdrive_client_secret'] = array(
      '#type' => 'select',
      '#options' => $text_fields,
      '#title' => t('Client secret field'),
      '#default_value' => $droogle_og_clientsecret_field,
      '#description' => t('Select the text field on a group content type that will contain the overriding client secret for that organic group.'),
    );

    $form['single_groups']['droogle_og_field_gdrive_refresh_token'] = array(
      '#type' => 'select',
      '#options' => $text_fields,
      '#title' => t('Refresh Token field'),
      '#default_value' => $droogle_og_field_gdrive_refresh_token,
      '#description' => t('Select the text field on a group content type that will contain the overriding refresh token for that organic group.'),
    );
  }
  $form = system_settings_form($form);
  $form ['actions']['refresh'] = array(
    '#type' => 'submit',
    '#value' => $refresh_token_label,
    '#submit' => array(
      'system_settings_form_submit',
      'droogle_settings_form_refresh_token_submit'
    ),
  );
  return $form;
}

/**
 * Refresh the token.
 */
function droogle_settings_form_refresh_token_submit($form, &$form_state) {
  $results = droogle_get_google_token(array('set_token' => TRUE, 'destination' => 'admin/config/content/droogle'));
  if (!empty($results['redirect'])) {
    $form_state['redirect'] = array(
      $results['redirect'],
      array(
        'external' => TRUE,
      ),
    );
  }
}

/**
 * Page for generation the refresh token.
 */
function droogle_get_google_token_page() {
  $connection_info = droogle_get_connection_settings();
  $client_secret = $connection_info['client_secret'];
  // Ensure a proper token has been passed.
  $state = array();
  if (isset($_GET['code'])) {
    if (!empty($_GET['state'])) {
      $state = unserialize($_GET['state']);
    }
    $token = empty($state['token']) ? NULL : $state['token'];
    if (!$token || !drupal_valid_token($token, $client_secret)) {
      return drupal_access_denied();
    }
  }

  $result = droogle_get_google_token();
  if (!empty($result['refresh_token'])) {
    // If configured to, set the token and return to original page.
    if (!empty($state['set_token'])) {
      variable_set('droogle_refresh_token', $result['refresh_token']);
      if (!empty($state['destination'])) {
        drupal_set_message(t('Refresh token configured to: !token', array('!token' => $result['refresh_token'])));
        drupal_goto($state['destination']);
        return;
      }
    }
    return t('Refresh token')
    . ':<pre>' . $result['refresh_token'] . '</pre>'
    . '<br/>'
    . t('Access token')
    . ':<pre>' . $result['access_token'] . '</pre>';
  }
  elseif (!empty($result['redirect'])) {
    drupal_goto($result['redirect'], array('external' => TRUE));
  }
  else {
    return drupal_not_found();
  }
}
