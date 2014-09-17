<?php
// $Id: theme-settings.php,v 1.7 2008/09/11 09:36:50 johnalbin Exp $

// Include the definition of zen_settings() and zen_theme_get_default_settings().
include_once './' . drupal_get_path('theme', 'zen') . '/theme-settings.php';


/**
 * Implementation of THEMEHOOK_settings() function.
 *
 * @param $saved_settings
 *   An array of saved settings for this theme.
 * @return
 *   A form array.
 */
function austin_settings($saved_settings) {

  // Get the default values from the .info file.
  $defaults = zen_theme_get_default_settings('austin');

  // Merge the saved variables and their default values.
  $settings = array_merge($defaults, $saved_settings);

  /*
   * Create the form using Forms API: http://api.drupal.org/api/6
   */
  $form = array();
  
  $form['austin_color'] = array(
    '#type'          => 'fieldset',
    '#title'         => t('Color settings'),
    '#attributes'    => array('id' => 'austin-colors'),
  );
  
  $form['austin_color']['austin_color_option'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Choose the alternate color palette'),
    '#default_value' => $settings['austin_color_option'],
    '#description'   => t('A body class of .with-alternate-color will be added to the $body_classes variable.'),
    '#prefix'        => '<div id="div-austin-colors"><strong>' . t('Blue/Red color palette:') . '</strong>',
    '#suffix'        => '</div>',
  );
  
  // */

  // Add the base theme's settings.
  $form += zen_settings($saved_settings, $defaults);
  
  $form['zen_block_editing']['#weight'] = -4;
  
  // Remove some of the base theme's settings.
  unset($form['themedev']['zen_layout']); // We don't need to select the base stylesheet.

  // Return the form
  return $form;
}
