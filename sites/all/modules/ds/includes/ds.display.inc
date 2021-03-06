<?php

/**
 * @file
 * Display overview form.
 */

/**
 * General overview page.
 */
function ds_layout_overview() {
  $content = '';

  drupal_add_css(drupal_get_path('module', 'ds') .'/css/ds.css');

  $rows = array();
  foreach (module_implements('ds_api') as $module) {
    if (user_access('configure layout for '. $module)) {
      $api_info = ds_api_info($module);

      // Build modes.
      $build_modes = ds_get_build_modes($module);

      foreach ($api_info['types']() as $tkey => $otype) {

        $type_name = $otype->type;
        $type = $type_name;
        $type_url_str = str_replace('_', '-', $type_name);

        // Global exclude.
        if (variable_get($module .'_type_'. $type, FALSE)) {
          continue;
        }

        $exclude_build_modes = variable_get($module .'_buildmodes_exclude', array());

        if (!empty($build_modes)) {

          $build_mode_rows = array();
          foreach ($build_modes as $bkey => $build_mode) {

            $excluded = (isset($exclude_build_modes[$type_name][$bkey]) && $exclude_build_modes[$type_name][$bkey] == TRUE) ? TRUE : FALSE;
            if ($excluded) {
              continue;
            }

            // Settings en status.
            $display_settings = variable_get($module .'_display_settings_'. $type, array());
            $settings_status = isset($display_settings[$bkey]['status']) ? $display_settings[$bkey]['status'] : DS_SETTINGS_UI;

            $row = array();
            $row[] = $build_mode['title'];
            $row[] = ds_settings_status($settings_status);

            $operations = l('Edit', DS_PATH_LAYOUT .'/'. $type_url_str .'/'. $bkey);
            if (user_access('revert overridden settings') && $settings_status == DS_SETTINGS_OVERRIDDEN) {
              $operations .= ' - '. l('Revert', DS_PATH_TOOLS .'/revert/'. $module .'/'. $type_url_str .'/'. $bkey);
            }
            $row[] = $operations;
            $build_mode_rows[] = array('data' => $row, 'class' => 'ds-row-type');
          }

          // Any build modes found ?
          if (!empty($build_mode_rows)) {
            $rows[] = array(array('data' => check_plain(drupal_ucfirst($otype->name)), 'colspan' => '3', 'class' => 'ds-row-title'));
            $rows = array_merge($rows, $build_mode_rows);
          }
        }
      }
    }
  }

  // Special listing for views which are using the views fields style plugin.
  if (module_exists('views')) {
    $views_rows = array();
    $views_ui = (module_exists('views_ui') && user_access('administer views'));
    $views = views_get_all_views();
    foreach ($views as $v => $view) {
      foreach ($view->display as $d => $display) {
        if (isset($display->display_options['row_plugin']) && $display->display_options['row_plugin'] == 'ds_fields') {
          $views_row = array();
          $views_row[] = ucfirst($view->name);
          $views_row[] = '';
          if ($views_ui) {
            $views_row[] = l(t('Edit'), 'admin/build/views/edit/'. $view->name);
          }
          else {
            $views_row[] = '';
          }
          $views_rows[] = array('data' => $views_row, 'class' => 'ds-row-type');
          break;
        }
      }
    }
    if (!empty($views_rows)) {
      $views_fields_explanation = 'This list of views are using the Display Suite fields plugin to render fields in regions.';
      if ($views_ui) {
        $views_fields_explanation .= ' The edit link will lead you to the edit screen of the view.';
      }
      else {
        $views_fields_explanation .= ' Views interface is disabled or you do not have access to edit the view.';
      }
      // Add empty row first and then the header of the views.
      $rows[] = array(array('data' => '&nbsp;', 'colspan' => '3', 'class' => 'ds-row-type'));
      $rows[] = array(array('data' => t('Views fields'), 'colspan' => '3', 'class' => 'ds-row-title'));
      $rows[] = array(array('data' => t($views_fields_explanation), 'colspan' => '3'));
      // Views rows
      $rows = array_merge($rows, $views_rows);
    }
  }

  if (!empty($rows)) {
    $content = theme('table', array(), $rows, array('id' => 'ds-overview-page'));
  }
  else {
    $content = '<p>'. t('No modules which provide Displays were found.') .'</p></p>'. t('For example, to change display options for nodes, use <a href="http://drupal.org/project/nd">Node Displays</a>. Other modules are listed in the <a href="http://drupal.org/node/644662">documentation</a>.') .'</p>';
  }

  return $content;
}

/**
 * Helper function to return readable name of status.
 *
 * @param boolean $status The status.
 */
function ds_settings_status($status) {
  $settings_status_descriptions = array(
    DS_SETTINGS_UI => t('Normal'),
    DS_SETTINGS_DEFAULT => t('Default'),
    DS_SETTINGS_OVERRIDDEN => t('Overridden'),
  );

  return $settings_status_descriptions[$status];
}

/**
 * Menu callback, show styles form.
 */
function ds_styles_form(&$form_state) {
  $form = array();

  $form['ds_styles'] = array(
    '#type' => 'textarea',
    '#title' => t('Styles for fields'),
    '#default_value' => variable_get('ds_styles', ''),
    '#description' => t('Configure styles which you can add to fields in the overview screen. Add multiple styles line per line.<br />If you want to have a friendly name, separate class and friendly name by |, but this is not required. eg:<br /><em>class_name_1<br />class_name_2|Friendly name<br />class_name_3</em><br />To group styles in the overview screen, use any number of tildes (~) as class name and the friendly name will be used as the group label in the styles selector. See <a href="http://drupal.org/node/1176182#comment-4578530">this issue</a> for more information.')
  );

  $form['ds_styles_regions'] = array(
    '#type' => 'textarea',
    '#title' => t('Styles for regions'),
    '#default_value' => variable_get('ds_styles_regions', ''),
    '#description' => t('Configure styles which you can add to regions in the overview screen. Add multiple styles line per line.<br />If you want to have a friendly name, separate class and friendly name by |, but this is not required. eg:<br /><em>class_name_1<br />class_name_2|Friendly name<br />class_name_3</em>')
  );

  return system_settings_form($form);
}

/**
 * Menu callback; presents a listing of fields display settings for an object type.
 *
 * Form includes form widgets to select which fields appear for teaser, full node
 * etc and how the field labels should be rendered.
 *
 * This function is based on content_display_overview_form from
 * content.admin.inc of cck, but altered to have weights and regions.
 */
function ds_display_overview_form(&$form_state, $type, $build_mode = 'full', $module = 'nd') {

  // Gather information.
  $api_info = ds_api_info($module);
  $display_settings = ds_get_settings($module, $type, $build_mode);

  // See if this build mode isn't synced from another build mode.
  $synced = FALSE;
  $all_build_modes = ds_get_build_modes('nd');
  foreach ($all_build_modes as $build_mode_key => $value) {
    $values = ds_get_settings($module, $type, $build_mode_key);
    if (isset($values['sync'])) {
      foreach ($values['sync'] as $sync => $sync_value) {
        if ($sync == $build_mode && isset($all_build_modes[$build_mode_key])) {
          $synced = $build_mode_key;
          break;
        }
      }
    }
  }

  $settings_status = isset($display_settings['status']) ? $display_settings['status'] : DS_SETTINGS_UI;

  $form = array(
    '#synced' => FALSE,
    '#tree' => TRUE,
    '#module' => $module,
    '#type_name' => $type,
    '#fields' => array(),
    '#build_mode' => $build_mode,
    '#plugins' => FALSE,
    '#extra' => array(),
    '#status' => $settings_status,
    '#regions' => ds_regions(variable_get('ds_build_mode_'. $build_mode, 'all')),
  );

  if ($synced) {
    $form['#synced'] = t('You can not edit this buildmode right now because it is synchronised with !bm.', array('!bm' => l($all_build_modes[$synced]['title'], DS_PATH_LAYOUT .'/'. $type .'/'. $synced)));
  }

  // Build style selects for region classes.
  $region_style_options = ds_styles('regions');
  $region_styles = ds_default_value($display_settings, 'region_styles');
  foreach ($form['#regions'] as $key => $region_name) {
    if ($key != 'disabled') {
      $default_value = isset($region_styles[$key]) ? explode(' ', $region_styles[$key]) : array();
      $form['region_styles_'. $key] = array(
        '#type' => 'select',
        '#title' => t('Region style'),
        '#multiple' => TRUE,
        '#options' => $region_style_options,
        '#default_value' => $default_value,
        '#description' => t('Select styles to apply to this region.'),
        '#attributes' => array(
          'class' => 'ds-style-change',
        ),
      );
      $human_names = array();
      if (!empty($default_value)) {
        $human_names = _ds_styles_match_options($region_style_options, array_filter($default_value));
      }
      $form['region_styles_'. $key .'_summary'] = array(
        '#type' => 'markup',
        '#value' => implode(', ', $human_names),
      );
    }
  }

  // Extra info.
  if (isset($api_info['extra']) && !empty($api_info['extra'])) {
    $types = $api_info['types']();
    $type_info = $types[$type];
    foreach ($api_info['extra'] as $key) {
      $form['#extra'][$key] = $type_info->{$key};
    }
  }

  // Add the fields.
  ds_fields_display_form($form, $display_settings);

  // Sync / copy tab.
  ds_sync_copy_form($form, $display_settings, $build_mode, $module, $type);

  // Plugins.
  $plugins = TRUE;
  if (isset($api_info['plugins_exclude']) && in_array($build_mode, $api_info['plugins_exclude'])) {
    $plugins = FALSE;
  }
  if ($plugins) {
    ds_plugins_display_form($form, $display_settings);
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 * Add fields to display overview form.
 */
function ds_fields_display_form(&$form, $display_settings, $fields = array()) {

  global $base_url;

  $module = $form['#module'];
  $build_mode = $form['#build_mode'];
  $type = $form['#type_name'];
  $extra = $form['#extra'];

  if (empty($fields)) {
    $fields = ds_get_fields($module, $type, $build_mode, $extra, TRUE, FALSE);
  }

  // Break form build in case no fields.
  if (empty($fields)) {
    return;
  }

  $field_style_options = ds_styles('fields');
  $default_properties = ds_field_default_form_properties($build_mode);
  $default = (!empty($form[key($fields)])) ? $form[key($fields)] : $default_properties;

  foreach ($fields as $field => $value) {    
    $summary = '';

    // Add form variable to let the template system know about the fields.
    $form['#fields'][] = $field;

    // Every field starts with the default value
    $form[$field] = $default;

    // Store the type.
    $form[$field][$build_mode]['type'] = array('#type' => 'value', '#value' => $value['type']);

    // Field key, parent key and depth.
    $parent = ds_default_value($display_settings, 'fields', $field, 'parent', '');
    if (!isset($fields[$parent])) {
      $parent = '';
    }
    $form[$field][$build_mode]['#depth'] = (!empty($parent)) ? 1 : 0;
    $form[$field][$build_mode]['parent_id'] = array(
      '#type' => 'hidden',
      '#default_value' => $parent,
      '#attributes' => array('class' => 'ds-parent-id'),
      '#size' => 20,
    );
    $form[$field][$build_mode]['field_id'] = array(
      '#type' => 'hidden',
      '#value' => $field,
      '#size' => 20,
      '#attributes' => array('class' => 'ds-field-id'),
    );

    // Populate/override the fields with the saved values
    $label_format = isset($value['display_settings'][$build_mode]['label']['format']) ? $value['display_settings'][$build_mode]['label']['format'] : ds_default_value($display_settings, 'fields', $field, 'labelformat', DS_DEFAULT_LABEL_FORMAT);
    $form[$field][$build_mode]['label']['format']['#default_value'] = $label_format;
    $summary .= '<span class="label-info">Label: '. $label_format .'</span>';

    // Labels.
    $label_value = ds_default_value($display_settings, 'fields', $field, 'label_value', '');
    $form[$field][$build_mode]['label_value'] = array(
      '#type' => 'textfield',
      '#title' => t('Label text'),
      '#default_value' => $label_value,
      '#description' => (!empty($label_value)) ? t('The original value was: @label', array('@label' => $value['title'])) : t('Change the default label text, which will be translatable.'),
    );

    // Label.
    $form[$field]['human_name']['#value'] = !empty($label_value) ? check_plain($label_value) : (isset($value['label']) ? $value['label'] : $value['title']);
    $form[$field]['field_name']['#value'] = check_plain($field);
    
    // Weight.
    $form[$field]['weight']['#default_value'] = ds_default_value($display_settings, 'fields', $field, 'weight', DS_DEFAULT_WEIGHT);
    $form[$field]['ds_weight']['#default_value'] = ds_default_value($display_settings, 'fields', $field, 'weight', DS_DEFAULT_WEIGHT);

    // Formatting.
    $form[$field][$build_mode]['format']['#options'] = isset($value['properties']['formatters']) ? $value['properties']['formatters'] : array();
    $format = ds_default_value($display_settings, 'fields', $field, 'format', '');
    if (empty($format) && isset($value['properties']['formatters'])) {
      $format = key($value['properties']['formatters']);
    }
    $form[$field][$build_mode]['format']['#default_value'] = $format;
    $form[$field][$build_mode]['format']['#access'] = (count($form[$field][$build_mode]['format']['#options']) == 0) ? FALSE : TRUE;

    // Only show group formatting if the option is available
    if (isset($value['properties']['subgroup_formatters'])) {
      $form[$field][$build_mode]['subgroup_format']['#options'] = $value['properties']['subgroup_formatters'];
      $subgroup_format = ds_default_value($display_settings, 'fields', $field, 'subgroup_format', '');
      if (empty($subgroup_format) && isset($value['properties']['formatters'])) {
        $subgroup_format = key($value['properties']['formatters']);
      }
      $form[$field][$build_mode]['subgroup_format']['#default_value'] = $subgroup_format;
      $form[$field][$build_mode]['subgroup_format']['#access'] = (count($form[$field][$build_mode]['format']['#options']) == 0) ? FALSE : TRUE;
    }
    else {
      unset($form[$field][$build_mode]['subgroup_format']);
    }
    
    if (!empty($format)) {
      $summary .= ' - <span class="format-info">Format: '. $form[$field][$build_mode]['format']['#options'][$format] .'</span>';
    }

    // Class.
    $class = ds_default_value($display_settings, 'fields', $field, 'css-class', '');
    $class_values = explode(' ', $class);
    $human_names = array();
    if (!empty($class_values)) {
      $human_names = _ds_styles_match_options($field_style_options, array_filter($class_values));
    }
    $form[$field][$build_mode]['css-class']['#default_value'] = explode(' ', $class);
    if (!empty($human_names)) {
      $summary .= '<span class="style-info"> - Styles: '. implode(', ', $human_names) .'</span>';
    }
    else {
      $summary .= '<span class="style-info"></span>';
    }

    // Region.
    $form[$field][$build_mode]['region']['#default_value'] = ds_default_value($display_settings, 'fields', $field, 'region', DS_DEFAULT_REGION);

    // Settings summary.
    $form[$field][$build_mode]['summary'] = array(
      '#type' => 'markup',
      '#value' => $summary,
    );
  }
}

/**
 * Match option labels from specified option values.
 */
function _ds_styles_match_options($options, $values) {
  $matches = array();
  foreach ($options as $optkey => $optvalue) {
    if (is_array($optvalue)) {
      $matches = array_merge($matches, _ds_styles_match_options($optvalue, $values));
    }
    elseif (in_array($optkey, $values, TRUE)) {
      $matches[] = $optvalue;
    }
  }
  return $matches;
}

/**
 * Add the sync / copy tab to the display overview form.
 *
 * @param $form The display form.
 * @param array $display_settings Current display settings.
 * @param array $current_build_mode Current build mode.
 * @param string $module The current module.
 * @param string $type_name The name of the type.
 */
function ds_sync_copy_form(&$form, $display_settings, $current_build_mode, $module, $type_name) {
  $copy = array();
  $sync = array();
  $build_modes = array();
  $all_build_modes = ds_get_build_modes($module);
  $exclude_build_modes = variable_get($module .'_buildmodes_exclude', array());

  foreach ($all_build_modes as $key => $build_mode) {
    $excluded = (isset($exclude_build_modes[$type_name][$key]) && $exclude_build_modes[$type_name][$key] == TRUE) ? TRUE : FALSE;
    if ($excluded || $key == $current_build_mode) {
      continue;
    }
    $copy[$key] = $build_mode['title'];
    $sync[$key] = t('Keep synchronized');
  }

  if (!empty($copy)) {
    $form['sync_copy'] = array(
      '#type' => 'fieldset',
      '#description' => t('Copy once or always sync display and plugin settings with other build modes. Syncing options are not saved bi-directional.<br />Watch out when copying field settings to another build mode which has a different set of regions as you can loose some fields for display. A typical example is RSS which only has one region (middle).'),
    );

    $form['sync_copy']['info'] = array(
      '#type' => 'item',
      '#value' => t('Copy "!current" to', array('!current' => $all_build_modes[$current_build_mode]['title'])),
      '#prefix' => '<div style="float: left; margin-right: 10px;">',
      '#suffix' => '</div>',
    );

    $form['sync_copy']['copy'] = array(
      '#type' => 'checkboxes',
      '#options' => $copy,
      '#prefix' => '<div style="float: left; margin-right: 30px; margin-top: 10px;">',
      '#suffix' => '</div>',
    );

    $form['sync_copy']['sync'] = array(
      '#type' => 'checkboxes',
      '#options' => $sync,
      '#default_value' => isset($display_settings['sync']) ? $display_settings['sync'] : array(),
      '#prefix' => '<div style="float: left; margin-top: 10px;">',
      '#suffix' => '</div>',
    );
  }
}

/**
 * Add plugins to display overview form.
 *
 * @param array $form The display form.
 * @param array $display_settings Current display settings.
 */
function ds_plugins_display_form(&$form, $display_settings) {
  $module = $form['#module'];
  $plugins = variable_get($module .'_plugin_settings', array());
  if (!empty($plugins)) {
    $form['#plugins'] = TRUE;
    $form['#plugin_keys'] = array();

    foreach ($plugins as $key => $data) {
      if (isset($data['filename'])) {
        require_once($data['filename']);
      }
      $class = $data['class'];
      $plugin = new $class();
      $plugin_form = $plugin->plugin_form($form, $display_settings);
      $form['#plugin_keys'][$key] = $data['title'];
    }
  }
}

/**
 * Save fields & plugins for a build mode.
 */
function ds_display_overview_form_submit($form, &$form_state) {

  $module = $form['#module'];
  $build_mode = $form['#build_mode'];
  $type = $form['#type_name'];
  $extra = $form['#extra'];

  // Fields.
  $fields = ds_get_fields($module, $type, $build_mode, $extra, TRUE, FALSE);
  if (!empty($fields)) {
    foreach ($fields as $key => $field) {

      $region = $form_state['values'][$key][$build_mode]['region'];

      // Do not store these settings.
      if ($region == 'disabled') {
        // Ditch this field, unless it has its storage in another module (ie CCK)
        if (!isset($field['storage'])) {
          unset($form_state['values'][$key]);
        }
        continue;
      }

      $display_settings[$build_mode]['fields'][$key]['region'] = $region;
      $display_settings[$build_mode]['fields'][$key]['weight'] = $form_state['values'][$key]['ds_weight'];
      $display_settings[$build_mode]['fields'][$key]['format'] = $form_state['values'][$key][$build_mode]['format'];
      if (!empty($form_state['values'][$key][$build_mode]['subgroup_format'])){
        $display_settings[$build_mode]['fields'][$key]['subgroup_format'] = $form_state['values'][$key][$build_mode]['subgroup_format'];
      }
      $class = implode(' ', $form_state['values'][$key][$build_mode]['css-class']);
      $display_settings[$build_mode]['fields'][$key]['css-class'] = $class;

      // Label value.
      if (!empty($form_state['values'][$key][$build_mode]['label_value'])) {
        $display_settings[$build_mode]['fields'][$key]['label_value'] = $form_state['values'][$key][$build_mode]['label_value'];
      }
      $display_settings[$build_mode]['fields'][$key]['labelformat'] = $form_state['values'][$key][$build_mode]['label']['format'];

      // Parent.
      $field_id = $form_state['values'][$key][$build_mode]['field_id'];
      $parent_id = $form_state['values'][$key][$build_mode]['parent_id'];
      if (!empty($parent_id)) {
        $display_settings[$build_mode]['fields'][$key]['parent'] = $parent_id;
      }

      // Ditch this field, unless it has its storage in another module (ie CCK)
      if (!isset($field['storage']))
      unset($form_state['values'][$key]);
    }
  }

  // Plugins.
  if ($form['#plugins'] == TRUE) {
    ds_plugins_display_submit($form, $form_state, $display_settings);
  }

  // Status.
  $status = ($form['#status'] == DS_SETTINGS_DEFAULT) ? DS_SETTINGS_OVERRIDDEN : $form['#status'];
  $display_settings[$build_mode]['status'] = $status;

  // Sync / copy tab.
  ds_sync_copy_form_submit($form, $form_state, $display_settings);

  // Regions styles.
  $region_styles = array();
  foreach ($form['#regions'] as $region_key => $region_name) {
    if ($region_key != 'disabled')
    $region_styles[$region_key] = implode(' ', $form_state['values']['region_styles_'. $region_key]);
  }
  $display_settings[$build_mode]['region_styles'] = $region_styles;

  // Remove old settings.
  db_query("DELETE FROM {ds_settings} WHERE build_mode = '%s' AND module = '%s' AND type = '%s'", $build_mode, $module, $type);

  // Save new settings.
  $record = new stdClass();
  $record->module = $module;
  $record->type = $type;
  $record->build_mode = $build_mode;
  $record->settings = serialize($display_settings[$build_mode]);
  drupal_write_record('ds_settings', $record);

  // Check if we need to copy or sync.
  unset($display_settings[$build_mode]);
  if (!empty($display_settings)) {
    foreach ($display_settings as $sync_build_mode => $values) {

      // Remove old settings for this sync/copy build mode.
      db_query("DELETE FROM {ds_settings} WHERE build_mode = '%s' AND module = '%s' AND type = '%s'", $sync_build_mode, $module, $type);

      // Save new sync/copy settings.
      $record = new stdClass();
      $record->module = $module;
      $record->type = $type;
      $record->build_mode = $sync_build_mode;
      $record->settings = serialize($values);
      drupal_write_record('ds_settings', $record);
    }
  }

  // Confirmation message.
  drupal_set_message(t('Your settings have been saved.'));
}

/**
 * Copy / sync settings.
 *
 * @param array $form The display form.
 * @param array $form_state The submitted values of the form.
 * @param array $display_settings Current display settings being saved.
 */
function ds_sync_copy_form_submit($form, $form_state, &$display_settings) {
  $sync_copy = _ds_get_sync_copy($form, $form_state);
  if (!empty($sync_copy)) {
    $current_build_mode = $form['#build_mode'];
    foreach ($sync_copy['copy'] as $key => $value) {
      if ($key === $value || $sync_copy['sync'][$key] === $key) {

        // Copy.
        $display_settings[$key] = $display_settings[$current_build_mode];

        // Keep in sync ?
        if (isset($sync_copy['sync'][$key])) {
          $display_settings[$current_build_mode]['sync'][$key] = $key;
        }
      }
    }
  }
}

/**
 * Helper function to get build modes which need to be synced / copied.
 */
function _ds_get_sync_copy($form, $form_state) {
  static $checked = FALSE;
  static $sync_copy = array();
  if (isset($form_state['values']['sync_copy']) && $checked == FALSE) {
    $checked = TRUE;
    $current_build_mode = $form['#build_mode'];
    $values = $form_state['values']['sync_copy'];
    foreach ($values['copy'] as $key => $value) {
      if ($key === $value || $values['sync'][$key] === $key) {

        // Copy.
        $sync_copy['copy'][$key] = $key;

        // Keep in sync ?
        if ($values['sync'][$key] === $key) {
          $sync_copy['sync'][$key] = $key;
        }
      }
    }
  }
  return $sync_copy;
}

/**
 * Save plugins settings.
 *
 * @param array $form The display form.
 * @param array $form_state The submitted values of the form.
 * @param array $display_settings Current display settings being saved.
 */
function ds_plugins_display_submit($form, $form_state, &$display_settings) {
  $module = $form['#module'];

  $plugins = variable_get($module .'_plugin_settings', array());

  foreach ($plugins as $key => $data) {

    if (isset($data['filename'])) {
      require_once($data['filename']);
    }

    $class = $data['class'];
    $plugin = new $class();
    $plugin->plugin_form_submit($form, $form_state, $display_settings);
  }
}

/**
 * Return array of available label options.
 */
function ds_label_options() {
  $options = array(
    'above' => t('Above'),
    'inline' => t('Inline'),
    'hidden' => t('Hidden'),
  );

  // Give modules a chance to alter label options.
  drupal_alter('ds_label_options', $options);

  return $options;
}

/**
 * Return styles.
 */
function ds_styles($variable = '') {
  static $run = FALSE;
  static $styles = array();

  if (!$run) {
    // Field styles.
    $field_styles = _ds_style_parse_variable('ds_styles');
    $styles['fields'] = $field_styles;

    // Region styles.
    $region_styles = _ds_style_parse_variable('ds_styles_regions');
    $styles['regions'] = $region_styles;
    $run = TRUE;
  }

  return (empty($variable)) ? $styles : (isset($styles[$variable]) ? $styles[$variable] : array());
}

 /**
 * Parse style variable value into a keyed array suitable for FAPI options,
 * including the "None" option.
 */
function _ds_style_parse_variable($variable) {
  static $optgroup_key = '~';
  static $optgroup_label_trim = ' []';
  $styles = array('' => t('None'));

  $var_values = trim(variable_get($variable, ''));
  if (!empty($var_values)) {
    // Prepare loop variables
    $var_styles = explode("\n", $var_values);
    $label = NULL;
    $group = &$styles;
    foreach ($var_styles as $key => $value) {
      $classes = explode('|', trim($value));
      $key = trim($classes[0]);
      $name = isset($classes[1]) ? trim($classes[1]) : $key;
      // Group
      if (preg_match('/^(' . $optgroup_key . ')+$/', $key)) {
        // Accumulate this new group
        $label = trim($name, $optgroup_label_trim);
        $group = &$styles[$label];
     }
      // Regular style
      else {
        $group[$key] = $name;
      }
    }
  }
  return $styles;
}

/**
 * Function to load default form properties for a field in a context
 */
function ds_field_default_form_properties($build_mode) {

  $field = array(
    'label' => array(
      'format' => array(
        '#title' => t('Label position'),
        '#type' => 'select',
        '#options' => ds_label_options(),
        '#default_value' => DS_DEFAULT_LABEL_FORMAT,
        '#description' => t('Position of the label'),
        '#attributes' => array(
          'class' => 'ds-label-change'
        ),
      ),
    ),
    'format' => array(
      '#title' => t('Field format'),
      '#type' => 'select',
      '#options' => array(),
      '#default_value' => DS_DEFAULT_FORMAT,
      '#description' => t('Hide the label or select a style.'),
      '#attributes' => array(
        'class' => 'ds-format-change',
      ),
    ),
    'css-class' => array(
      '#title' => t('Field style'),
      '#type' => 'select',
      '#multiple' => TRUE,
      '#size' => 5,
      '#options' => ds_styles('fields'),
      '#default_value' => array(),
      '#description' => t('Select styles to apply to this field.'),
      '#attributes' => array(
        'class' => 'ds-style-change',
      ),
    ),
    'region' => array(
      '#type' => 'select',
      '#options' => ds_regions(variable_get('ds_build_mode_'. $build_mode, 'all')),
      '#default_value' => DS_DEFAULT_REGION,
      '#attributes' => array(
        'class' => 'field-region-select',
      ),
    ),
  );

  // If build mode is not empty, this means we're looking at
  // a content type which has no cck fields defined. Add
  // ds_weight and build mode key to the default field array
  // so the draghandling & formatters start working.
  if (!empty($build_mode)) {
    $field['ds_weight'] = array(
      '#type' => 'weight',
      '#default_value' => DS_DEFAULT_WEIGHT,
      '#delta' => 100,
    );
    $field[$build_mode] = array(
      'label' => array(
        'format' => array(
          '#title' => t('Label position'),
          '#type' => 'select',
          '#options' => ds_label_options(),
          '#default_value' => DS_DEFAULT_LABEL_FORMAT,
          '#description' => t('Hide the label or select a style.'),
          '#attributes' => array(
            'class' => 'ds-label-change'
          ),
        ),
      ),
      'format' => array(
        '#title' => t('Field format'),
        '#type' => 'select',
        '#options' => array(),
        '#default_value' => DS_DEFAULT_FORMAT,
        '#description' => t('Select how this field will be displayed.'),
        '#attributes' => array(
          'class' => 'ds-format-change',
        ),
      ),
      'subgroup_format' => array(
        '#title' => t('Subgroup format'),
        '#type' => 'select',
        '#options' => array(),
        '#default_value' => DS_DEFAULT_FORMAT,
        '#description' => t('Table formatters will ignore field formatter settings. Ensure that CCK\'s default <em>Subgroup format</em> for this group is set to <em>fieldset</em>.'),
        '#attributes' => array(
          'class' => 'ds-subgroup-format-change',
        ),
      ),
      'css-class' => array(
        '#title' => t('Field style'),
        '#type' => 'select',
        '#multiple' => TRUE,
        '#options' => ds_styles('fields'),
        '#default_value' => array(),
        '#description' => t('Select styles to apply to this field.'),
        '#attributes' => array(
          'class' => 'ds-style-change',
        ),
      ),
      'region' => array(
        '#type' => 'select',
        '#options' => ds_regions(variable_get('ds_build_mode_'. $build_mode, 'all')),
        '#default_value' => DS_DEFAULT_WEIGHT,
        '#attributes' => array(
          'class' => 'field-region-select',
        ),
      ),
    );
  }

  return $field;
}
