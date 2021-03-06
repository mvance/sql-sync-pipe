<?php

/**
 * Implementation of hook_drush_command().
 */
function sql_sync_pipe_drush_command() {
  $items['sql-sync-pipe'] = array(
    'bootstrap' =>  DRUSH_BOOTSTRAP_DRUSH,
    'description' => 'Copy and import source database to target database. Transfers as a gzipped pipe via SSH.',
		'aliases' => array(
      'ssp',
			'pipe',
		),
    'arguments' => array(
      'source' => 'Name of the site-alias to use as a source.',
      'destination' => 'Name of the site-alias to use as the destination.',
    ),
    'options' => array(
      'dump' => 'Dump to a local file, this will help determine an ETA if the using the progress option.',
      'progress' => 'If the Pipe Viewer command is installed, show the progress of the sync. See: http://www.ivarch.com/programs/pv.shtml. Common installs: UNIX - "yum install pv" or "apt-get install pv", MacPorts - "port install pv", HomeBrew - "brew install pv". See URL for a complete list of installation instructions.',
      'temp' => 'Use a temporary file to hold dump files.',
      'ssh-options' => 'A string of extra options that will be passed to the ssh command (e.g. "-p 100")',
      'sanitize' => 'Obscure email addresses and reset passwords in the user table post-sync. Optional.',
      'enable' => 'Enable the specified modules in the target database after the sync operation has completed.',
      'disable' => 'Disable the specified modules in the target database after the sync operation has completed.',
      'permission' => 'Add or remove permissions from a role in the target database after the sync operation has completed. The value of this option must be an array, so it may only be specified in a site alias record or drush configuration file.  See `drush topic docs-example-sync-extension`.',
    ),
    'sub-options' => array(
      'sanitize' => array(
        'sanitize-password' => 'The password to assign to all accounts in the sanitization operation, or "no" to keep passwords unchanged.  Default is "password".', 
        'sanitize-email' => 'The pattern for test email addresses in the sanitization operation, or "no" to keep email addresses unchanged.  May contain replacement patterns %uid, %mail or %name.  Default is "user+%uid@localhost".', 
        'confirm-sanitizations' => 'Prompt yes/no after importing the database, but before running the sanitizations',
      ),
    ),
  );
  return $items;
}

function drush_sql_sync_pipe($source = NULL, $destination = NULL) {
 	drush_command_include('sql-sync');
  drush_include(DRUSH_BASE_PATH . '/commands/sql', 'sql.drush.inc');
  
  $source_settings = drush_sitealias_overlay_options(drush_sitealias_get_record($source), 'source-');
  $source_db_url = drush_sitealias_get_db_spec($source_settings, FALSE, 'source-');
  
  $destination_settings = drush_sitealias_overlay_options(drush_sitealias_get_record($destination), 'target-');
  $target_db_url = drush_sitealias_get_db_spec($destination_settings, FALSE, 'target-');

  if (empty($source_db_url)) {
    if (empty($source_settings)) {
      return drush_set_error('DRUSH_ALIAS_NOT_FOUND', dt('Error: no alias record could be found for !source', array('!source' => $source)));
    }
    return drush_set_error('DRUSH_DATABASE_NOT_FOUND', dt('Error: no database record could be found for !source', array('!source' => $source)));
  }
  if (empty($target_db_url)) {
    if (empty($destination_settings)) {
      return drush_set_error('DRUSH_ALIAS_NOT_FOUND', dt('Error: no alias record could be found for !destination', array('!destination' => $destination)));
    }
    return drush_set_error('DRUSH_DATABASE_NOT_FOUND', dt('Error: no database record could be found for !destination', array('!destination' => $destination)));
  }
    
  // Bootstrap to the source sites being sync'ed if it is local.
  // This allows modules enabled in the site to participate in the
  // sql-sync hook functions (e.g. to add sanitization operations, etc.).
  // If the source is remote and the destination is local, then we
  // will determine the sanitization operations after the database
  // has been copied.
  if (!drush_get_option('deferred-sanitization', FALSE) && drush_get_option(array('sanitize', 'destination-sanitize'), FALSE)) {
    $bootstrapped = drush_bootstrap_max_to_sitealias($source_settings);
    if ($bootstrapped) {
      drush_command_invoke_all('drush_sql_sync_sanitize', $source);
      drush_sql_sanitize();
    }
    else {
      drush_set_option('deferred-sanitization', TRUE);
    }
  }

  $target_dump = FALSE;
  if (drush_get_option('dump', FALSE)) {
    $target_dump = drush_sql_dump_file($destination_settings);
    if (drush_get_option('temp', FALSE)) {
      $target_dump = drush_tempnam($source_db_url['database'] . ($source_db_url['database'] == $target_db_url['database'] ? '' : '-to-' . $target_db_url['database']) . '.sql.');
    }
  }
  $progress = FALSE;
  if (drush_get_option('progress', FALSE)) {
    // Destination is local.
    if (empty($target_db_url['remote-host'])) {
      drush_shell_exec('which pv');
      $output = drush_shell_exec_output();
      if (!empty($output) && isset($output[0]) && file_exists($output[0])) {
        $progress = $output[0];
      }
    }
    else {
      $exec = drush_shell_proc_build($destination_settings, 'which pv');
      if (drush_shell_exec($exec)) {
        $output = drush_shell_exec_output();
        if (!empty($output) && isset($output[0])) {
          $progress = $output[0];
        }
      }
    }
  }
  
  $txt_source = (isset($source_db_url['remote-host']) ? $source_db_url['remote-host'] . '/' : '') . $source_db_url['database'];
  $txt_destination = (isset($target_db_url['remote-host']) ? $target_db_url['remote-host'] . '/' : '') . $target_db_url['database'];
  
  // Create the drop/create MySQL command.Re-create the local DB table.
  drush_print(dt('You will destroy data in !target and replace with data from !source.', array(
    '!source' => $txt_source,
    '!target' => $txt_destination,
  )));
  // If any sanitization operations are to be done, then get the
  // sanitization messages and print them as part of the confirmation.
  // If --sanitize was specified but there were no sanitize messages,
  // then warn that sanitization operations will be accumulated and
  // processed after the sync completes.
  $messages = _drush_sql_get_post_sync_messages();
  if ($messages) {
    drush_print();
    drush_print($messages);
  }
  else if (drush_get_option('deferred-sanitization', FALSE) && !drush_get_option('confirm-sanitizations', FALSE)) {
    drush_print();
    drush_print("WARNING: --sanitize was specified, but deferred (e.g. the source site is remote).  The sanitization operations will be determined after the database is copied to the local system and will be run without further confirmation.  Run with --confirm-sanitizations to force confirmation after the sync.");
  }
  // TODO: actually make the backup if desired.
  drush_print();
  drush_print(dt("You might want to make a backup first, using the sql-dump command.\n"));
  if (!drush_confirm(dt('Do you really want to continue?'))) {
    return drush_user_abort();
  }
  $commands = array();
  $commands[] = 'mysql';
  $commands[] = '-h ' . $target_db_url['host'];
  $commands[] = '-u ' . $target_db_url['username'];
  $commands[] = '-p' . $target_db_url['password'];
  $commands[] = '-e "DROP DATABASE \`' . $target_db_url['database'] . '\`; CREATE DATABASE \`' . $target_db_url['database'] . '\`;"';
  $cmd = implode(' ', $commands);
  // Destination is remote, wrap it a SSH.
  if (!empty($target_db_url['remote-host'])) {
    $cmd = drush_shell_proc_build($destination_settings, $cmd);
  }
  drush_shell_exec($cmd);

  // Create the gzipped SQL dump command.
  $dump_cmd = 'mysqldump -CceKq --single-transaction -h ' . $source_db_url['host'] . ' -u ' . $source_db_url['username'] . ' -p\'' . $source_db_url['password'] . '\' ' . $source_db_url['database'] . ' | gzip -c9';

  $commands = array();
  // Source is local
  if (empty($source_db_url['remote-host'])) {
    $commands[] = $dump_cmd;
  }
  else {
    $commands[] = drush_shell_proc_build($source_settings, $dump_cmd);
  }
  // Dump to a file, gunzip and then import.
  if (drush_get_option('dump', FALSE) && $target_dump) {
    // Dump
    drush_print();
    drush_print(dt('Saving dump from !source ...', array(
      '!source' => $txt_source,
    )));
    $commands[] = '|';
    $commands[] = $progress ? $progress . ' -cW | gunzip -c > ' . $target_dump : 'gunzip -c > ' . $target_dump;
    $cmd = implode(' ', $commands);
    // Destination is remote, wrap it a SSH.
    if (!empty($target_db_url['remote-host'])) {
      $cmd = drush_shell_proc_build($destination_settings, $cmd);
    }
    drush_shell_exec($cmd);
    // Importing
    drush_print();
    drush_print(dt('Importing dump into !target ...', array(
      '!target' => $txt_destination,
    )));
    $commands = array();
    if ($progress) {
      $commands[] = $progress . ' -cW ' . $target_dump . ' |';
    }
    $commands[] = 'mysql';
    $commands[] = '-u ' . $target_db_url['username'];
    $commands[] = '-p' . $target_db_url['password'];
    $commands[] = $target_db_url['database'];
    if (empty($progress)) {
      $commands[] = '< ' . $target_dump;
    }
    $cmd = implode(' ', $commands);
    // Destination is remote, wrap it a SSH.
    if (!empty($target_db_url['remote-host'])) {
      $cmd = drush_shell_proc_build($destination_settings, $cmd);
    }
    drush_shell_exec($cmd);
    // Remove temporary files.
    if (drush_get_option('temp', FALSE)) {
      $commands = array();
      $commands[] = 'rm -f ' . $target_dump;
      $cmd = implode(' ', $commands);
      // Destination is remote, wrap it a SSH.
      if (!empty($target_db_url['remote-host'])) {
        $cmd = drush_shell_proc_build($destination_settings, $cmd);
      }
      drush_shell_exec($cmd);
    }
  }
  // This is a single piped import.
  else {
    $commands[] = '| gunzip';
    $commands[] = $progress ? '| ' . $progress . ' -cW |' : '|';
    $commands[] = 'mysql';
    $commands[] = '-u ' . $target_db_url['username'];
    $commands[] = '-p' . $target_db_url['password'];
    $commands[] = $target_db_url['database'];
    $cmd = implode(' ', $commands);
    // Destination is remote, wrap it a SSH.
    if (!empty($target_db_url['remote-host'])) {
      $cmd = drush_shell_proc_build($destination_settings, $cmd);
    }
    drush_shell_exec($cmd);
  }
  
  // After the database is imported into the destination, we
  // will check and see if we did not collect sanitization
  // operations at the beginning of the function (i.e. because the source
  // site was remote), and if the destination site is local,
  // then we will call the sanitization hooks now.
  // This presumes an important precondition, that the code
  // files were sync'ed before the database was sync'ed.
  if (drush_get_option('deferred-sanitization', FALSE) && (drush_has_boostrapped(DRUSH_BOOTSTRAP_DRUPAL_SITE) == FALSE)) {
    $bootstrapped = drush_bootstrap_max_to_sitealias($destination_settings);
    if ($bootstrapped) {
      drush_command_invoke_all('drush_sql_sync_sanitize', $destination);
      $messages = _drush_sql_get_post_sync_messages();
      if ($messages) {
        drush_print();
        drush_print($messages);
      }
      if (!drush_confirm(dt('Do you really want to sanitize the current database?'))) {
        return drush_user_abort();
      }
      drush_sql_bootstrap_further();
      drush_include(DRUSH_BASE_PATH . '/commands/sql', 'sync.sql');
      drush_command_invoke_all('drush_sql_sync_sanitize', 'default');
      $options = drush_get_context('post-sync-ops');
      $sanitize_query = '';
      foreach ($options as $id => $data) {
        $sanitize_query .= $data['query'] . " ";
      }
      if (!empty($sanitize_query)) {
        if (!drush_get_context('DRUSH_SIMULATE')) {
          drush_sql_query($sanitize_query);
        }
        else {
          drush_print("Executing: $sanitize_query");
        }
      }
    }
  }
}

function drush_sql_sync_pipe_post_sql_sync_pipe($source = NULL, $destination = NULL) {
  $modules_to_enable = drush_get_option_list('enable');
  if (!empty($modules_to_enable)) {
    drush_log(dt("Enable !modules", array('!modules' => implode(',', $modules_to_enable))), 'ok');
    drush_invoke_process($destination, 'pm-enable', $modules_to_enable, array('yes' => TRUE));
  }
  $modules_to_disable = drush_get_option_list('disable');
  if (!empty($modules_to_disable)) {
    drush_log(dt("Disable !modules", array('!modules' => implode(',', $modules_to_disable))), 'ok');
    drush_invoke_process($destination, 'pm-disable', $modules_to_disable, array('yes' => TRUE));
  }
  $permissions_table = drush_get_option('permission');
  if (!empty($permissions_table)) {
    foreach ($permissions_table as $role => $actions) {
      $values = drush_invoke_process($destination, 'sql-query', array("select perm from permission, role where role.name='$role' and role.rid = permission.rid;"), array(), array('integrate' => FALSE));
      // Remove the first line and explode
      $sql_output = preg_replace('/^.+\n/', '', trim($values['output']));
      $permissions = explode(', ', $sql_output);
      $original_permissions = $permissions;
      if (array_key_exists('add', $actions)) {
        $permissions_to_add = is_array($actions['add']) ? $actions['add'] : explode(', ', $actions['add']);
        drush_log(dt("Add !permissions to !role", array('!permissions' => implode(', ', $permissions_to_add), '!role' => $role)), 'ok');
        $permissions = array_unique(array_merge($permissions, $permissions_to_add));
      }
      if (array_key_exists('remove', $actions)) {
        $permissions_to_remove = is_array($actions['remove']) ? $actions['remove'] : explode(', ', $actions['remove']);
        drush_log(dt("Remove !permissions from !role", array('!permissions' => implode(', ', $permissions_to_remove), '!role' => $role)), 'ok');
        $permissions = array_diff($permissions, $permissions_to_remove);
      }
      if ($permissions != $original_permissions) {
        $permissions_string = implode(', ', $permissions);
        $values = drush_invoke_process($destination, 'sql-query', array("update permission, role set perm='$permissions_string' where role.name='$role' and role.rid = permission.rid;"), array(), array('integrate' => FALSE));
      }
    }
  }
}
