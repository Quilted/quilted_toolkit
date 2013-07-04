<?php

/**
 * Set the breadcrumb using a context reaction.
 */
class quilted_toolkit_context_reaction_breadcrumb_allow_hidden extends context_reaction_menu {
  /**
   * Override options_form().
   *
   * Add an option to use hidden menus.
   */
  function options_form($context) {
    if (module_exists('menu')) {
      $menus = menu_parent_options(menu_get_menus(), array('mlid' => 0));
      $root_menus = array();
      foreach ($menus as $key => $name) {
        $id = explode(':', $key);
        if ($id[1] == '0') {
          $root_menus[$id[0]] = check_plain($name);
        }
        else {
          $link = menu_link_load($id[1]);
          $identifier = $link['link_path'];
          $root_menu = $root_menus[$id[0]];
          while (isset($menus[$root_menu][$identifier])) {
            $identifier .= "'";
          }
          $menus[$root_menu][$identifier] = $name;
        }
        unset($menus[$key]);
      }
      array_unshift($menus, "-- " . t('None') . " --");
    }
    else {
      $menus = array();
    }

    $values = $this->fetch_from_context($context);
    $form = array(
      '#title' => $this->title,
      'menu' => array(
        '#title' => $this->title,
        '#description' => $this->description,
        '#options' => $menus,
        '#type' => 'select',
        '#default_value' => isset($values['menu']) ? $values['menu'] : '',
      ),
      'hidden' => array(
        '#title' => t('Allow hidden menus'),
        '#description' => t('Allow hidden menus to be used in breadcrumb trails.'),
        '#type' => 'checkbox',
        '#default_value' => isset($values['hidden']) ? $values['hidden'] : false,
      ),
    );
    return $form;
  }

  /**
   * Override of options_form_submit().
   * Trim any identifier padding for non-unique path menu items.
   */
  function options_form_submit($values) {
    foreach ($values as $key => $value) {
      $values[$key] = trim($value, "'");
    }
    return $values;
  }

  /**
   * Override of execute().
   */
  function execute(&$vars = NULL) {
    if ($active_paths = $this->get_active_paths()) {
      $breadcrumb = array(l(t('Home'), '<front>', array('purl' => array('disabled' => TRUE))));

      foreach ($active_paths as $path) {
        $use_hidden = false;
        if (array_key_exists('hidden', $path) && $path['hidden']) {
          $use_hidden = true;
          unset($path['hidden']);
        }

        if ($use_hidden) {
          $result = db_select('menu_links')
            ->fields('menu_links', array('p1', 'p2', 'p3', 'p4', 'p5', 'p6', 'p7', 'p8'))
            ->condition('link_path', $path)
            ->execute();
        }
        else {
          $result = db_select('menu_links')
            ->fields('menu_links', array('p1', 'p2', 'p3', 'p4', 'p5', 'p6', 'p7', 'p8'))
            ->condition('link_path', $path)
            ->condition('hidden', 0)
            ->execute();
        }
        while ($parents = $result->fetchAssoc()) {
          $set = FALSE;
          foreach (array_filter($parents) as $plid) {
            $parent = menu_link_load($plid);
            if ($parent && $parent['access'] && !empty($parent['title'])) {
              if ($use_hidden || (!$use_hidden && empty($parent['hidden']))) {
                $set = TRUE;
                $breadcrumb[] = l($parent['title'], $parent['href']);
              }
            }
          }
          // Only set the breadcrumb if one or more links were added to the
          // trail. If not, continue iterating through possible menu links.
          if ($set) {
            drupal_set_breadcrumb($breadcrumb);
            break;
          }
        }
      }
    }
  }
}

