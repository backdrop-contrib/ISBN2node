<?php
/**
 * @file
 * isbn2node internal functions
 */


/*
 * Get book data from external database
 *
 * @param string $isbn
 *   ISBN of book to look up
 *
 * @return array
 *   Array of values obtained from book database.
 */
function _isbn2book_fetch_data($isbn = '') {
  $config = config('isbn2node.settings');
  // @todo This data ought to be cached locally since ISBNdb only allows a certain number of calls each day.


  switch ($config->get('isbn2node_db')) {
    case 'isbndb':
      // Form the url.

      $url = str_replace(array('%access_key', '%isbn_value'),
        array($config->get('isbn2node_isbndb_access_key'), $isbn),
        ISBN2NODE_ISBNDB_URL);

      // Get and parse the XML.

      $xml = file_get_contents($url);
      $p = xml_parser_create();
      xml_parse_into_struct($p, $xml, $vals, $index);
      $vals['success'] = $vals['1']['attributes']['TOTAL_RESULTS'];
      xml_parser_free($p);
      break;

    case 'openlib':
      $url = str_replace('%isbn_value',
        $isbn,
        ISBN2NODE_OPENLIBRARY_ISBN_URL);

      // Get and parse the Json information from Open Library.
      $json = file_get_contents($url);
      //_isbn2node_debug('json',$json);
      $vals = json_decode($json);
      
      // The API will return 0 as numFound if no matching records are found,
      // and the number found if any are.
      if ($vals->numFound > 0) {
        $vals->success = TRUE;
      }
      else {
        $vals->success = FALSE;
      }

      break;
  }

  return $vals;
}


/*
 * Check that the settings have been set before allowing use of module forms.
 *
 * @param array &$form
 *   An array reference to the form.
 *
 * @return bool
 *   TRUE on success, FALSE on missing setup, with form altered to show error.
 */
function _isbn2node_check_settings(&$form) {
  $config = config('isbn2node.settings');
  $ok = TRUE;
  if (!$config->get('isbn2node_db')) {
    backdrop_set_message(t('You must You must select a book database to use before using this feature.'), 'error');

    $form['error'] = array(
      '#markup' => l(t('Select a database here'), 'admin/config/content/isbn2node', array('query' => array('destination' => 'node/add/isbn2node_book/isbn2node'))),
      '#weight' => 150,
    );

    $ok = FALSE;

  }
  elseif (($config->get('isbn2node_db') == 'isbndb') && !$config->get('isbn2node_isbndb_access_key')) {
    backdrop_set_message(t('You must enter an ISBNdb access key in the settings for this ISBN2node module before using this feature.'), 'error');

    $form['error'] = array(
      '#markup' => l(t('Enter your access key here'), 'admin/config/content/isbn2node', array('query' => array('destination' => 'node/add/isbn2node_book/isbn2node'))),
      '#weight' => 150,
    );

    $ok = FALSE;

  }
  return $ok;
}



/*
 * Return the fields we want from the XML returned from ISBNdb
 * and their corresponding Backdrop field names.
 *
 * @param array $fields
 *   If set, the attribute fields are returned, else return main fields.
 *
 * @return array
 *   An array of XML-fields and their corresponding Backdrop fields.
 */
function _isbn2node_isbndb_tags($fields = FALSE) {
  if ($fields) {
    return array(
      'EDITION_INFO' => 'isbn2node_edition',
      'PHYSICAL_DESCRIPTION_TEXT' => 'isbn2node_physical_description',
    );
  }
  else {
    return array(
      'TITLE' => 'title',
      'AUTHORSTEXT' => 'isbn2node_author',
      'PUBLISHERTEXT' => 'isbn2node_publisher',
      'DETAILS' => 'isbn2node_details',
    );
  }
}


/*
 * Return the fields we want from the Json returned from Open Library
 * and their corresponding Backdrop field names.
 *
 * @return array
 *   Array of Json-fields and their corresponding Backdrop fields within
 *   the ISBN2node-book content type.
 */
function _isbn2node_openlib_tags() {
  return array(
    'number_of_pages_median' => 'isbn2node_physical_description',
    'title' => 'title',
    'subtitle' => 'isbn2node_subtitle',
    'author_name' => array(0 => array('name' => 'isbn2node_author')),
    'publisher' => array(0 => array('name' => 'isbn2node_publisher')),
    'weight' => 'isbn2node_details',
    'publish_year' => 'isbn2node_year',
  );
}


/*
 * Fix ISBN to be digits (and possibly X) only by removing dashes and spaces.
 *
 * @param string $isbn 
 *   An ISBN.
 * @return string
 *   The sanitized ISBN.
 */
function _isbn2book_normalize_isbn($isbn) {
  return trim(str_replace(array(' ', '-'), '', $isbn));
}


/*
 * Find cover image
 *
 * @param string $isbn
 *   An ISBN.
 * @param string $size
 *   Desired cover image size (S / M / L).
 *
 * @return string | false
 *   URL of the cover file if found or FALSE if not.
 */
function _isbn2node_fetch_cover($isbn, $size = 'L') {

  // Form the url.
  $url = str_replace(array('%isbn', '%size'),
    array($isbn, $size),
    ISBN2NODE_OPENLIBRARY_COVER_URL);
  //_isbn2node_debug('image url', $url);

  // Try to get the size. Suppress the warning because there's no way to
  // catch it with an exception handler. We'll check validity later.
  $size = @getimagesize($url);

  // Openlibrary returns a 1x1 pixel image on failure. Anything bigger is OK.
  if (is_array($size) && $size[0] > 1) {
    return $url;
  }
  else {
    return FALSE;
  }
}


/*
 * Parse values from either of the book databases.
 *
 * @param object $vals 
 *   The book data obtained from the chosen database.
 * @param string $isbn
 *   The ISBN of the book we are working on.
 *
 * @return array
 *   Array of book data with fields corresponding to the isbn2node content type.
 */
function _isbn2node_parse($vals, $isbn) {
  //_isbn2node_debug('vals', $vals);

  $isbn2book_data = array();

  switch (config_get('isbn2node.settings', 'isbn2node_db')) {
    case 'isbndb':

      // Work on the returned values.
      if ($vals['1']['attributes']['TOTAL_RESULTS']) {

        $interesting = _isbn2node_isbndb_tags();

        foreach ($vals as $v) {

          // Get extra ISBN for backup cover check.

          if (is_array($v)) {
            if (isset($v['attributes']) && is_array($v['attributes'])) {
                          $a = $v['attributes'];
              if (is_array($a)) {
                if (isset($a['ISBN'])) {
                  $isbn2book_data['xisbn'] = $a['ISBN'];
                }
              }
            }
          }

          if (isset($interesting[$v['tag']])) {
            switch ($v['tag']) {
              case 'DETAILS':
                // _isbn2node_debug("v['attributes']", $v['attributes']);

                $fields = _isbn2node_isbndb_tags(TRUE);
                foreach ($fields as $xml_field => $backdrop_field) {
                  switch ($xml_field) {
                    case 'EDITION_INFO':
                      // Handle edition/year.

                      $parts = explode(';', $v['attributes'][$xml_field]);
                      if (isset($parts[1])) {
                        $isbn2book_data['isbn2node_year'] = trim($parts[1]);
                      }
                      $v['attributes'][$xml_field] = $parts[0];
                    default:
                      $isbn2book_data[$backdrop_field] = $v['attributes'][$xml_field];
                  }
                }
                break;
              default:
                if (isset($v['value'])) {
                  $isbn2book_data[$interesting[$v['tag']]] = $v['value'];
                }
                break;
            }
          }
        }
      }
      break;

    case 'openlib':

      if ($vals->success) {
        //_isbn2node_debug('$vals->success', $vals->success);

        // Assume the first record is the right one.
        $vals = $vals->docs[0]; 

        $interesting = _isbn2node_openlib_tags();
        //_isbn2node_debug('$interesting', $interesting);


        foreach ($interesting as $json => $backdrop) {
          //_isbn2node_debug("json $json -> backdrop", $backdrop);

          $value = '';
          if (is_array($backdrop)) {
            foreach ($backdrop as $js => $dr) {
              foreach ($dr as $j => $d) {
                if (isset($vals->$json)) {
                  $value = $vals->$json;
                  $value = $value[0];
                  $isbn2book_data[$d] = $value;
                }
              }
            }
          }
          else {
            if (isset($vals->$json)) {
              $value = $vals->$json;
            }
            switch ($backdrop) {
              case 'isbn2node_physical_description':
                $isbn2book_data[$backdrop] = t('@pages pages', array('@pages' => $value));
                break;
              case 'isbn2node_year':
	      // @todo handle multiple years properly
                $isbn2book_data[$backdrop] = (is_array($value) ? min($value) : $value);
                break;
              default:
                $isbn2book_data[$backdrop] = $value;
                break;
            }
          }
        }
      }
      break;
  }
  return $isbn2book_data;
}


/*
 * Make and save a node based on a fields array.
 *
 * @param array $fields
 *   Array containing data for fields for isbn2node_book content type.
 */
function _isbn2node_make_node($fields) {
  global $user;

  $node = new stdClass();
  $node->title = $fields['title'];
  $node->type = 'isbn2node_book';
  $node->created = time();
  $node->changed = time();
  $node->status = 1;
  $node->promote = 1;
  $node->sticky = 0;
  $node->uid = $user->uid;
  $node->language = 'en';
  $node->timestamp = time();
  $node->revision = 0;

  // Remove title.
  unset($fields['title']);

  // Handle the rest of the fields.
  foreach ($fields as $field => $value) {
    if ($field == 'isbn2node_cover') {
      $cover = array('fid' => 0);

      // Handle special structure for image field.
      foreach ($value as $k => $v) {
        $cover[$k] = $v;
      }
      $node->$field = array('und' => array(0 => $cover));
    }
    else {
      // Transfer normal field.
      $node->$field = array('und' => array(0 => array('value' => $value)));
    }
  }

  node_submit($node);
  node_save($node);

  if (isset($node->nid)) {
    backdrop_set_message(t('Created !title from ISBN @isbn.',
    array('!title' => l($node->title, 'node/' . $node->nid), '@isbn' =>
    $fields['isbn2node_isbn'])));
  }
  else {
    backdrop_set_message(t('Failed creating book from ISBN @isbn.',
    array('@isbn' => $fields['isbn2node_isbn'])));
  }

}


/*
 * Get cover file and create file object in database.
 *
 * @param string $isbn
 *   ISBN of book whose cover we are looking for.
 *
 * @return object
 *   Populated file object on success, empty file object (->fid = 0) on fail.
 */
function _isbn2node_save_cover($isbn, $xisbn = '') {
  // Create empty file object.
  $file = new stdClass();
  $file->fid = 0;

  // Use backup ISBN if set.
  $isbn = $xisbn ? $xisbn : $isbn;

  // Create URL and set filename of local file.
  $url = _isbn2node_fetch_cover($isbn);

  if ($url) {
    $parts = pathinfo($url);

    $filename = 'cover_' . $isbn . '.' . $parts['extension'];

    // Fetch cover file.
    $file_temp = file_get_contents($url);
    if ($file_temp) {
      // Create file in database.
      $file = file_save_data($file_temp, 'public://' . $filename, FILE_EXISTS_RENAME);
    }
  }

  return $file;
}


/*
 * Check for ISBN already in database.
 *
 * @param string $isbn 
 *   ISBN to look for.
 *
 * @return bool | array
 *   FALSE if none found, assoc array of results if any found.
 */
function _isbn2node_isbn_exists($isbn) {
  return db_select('field_data_isbn2node_isbn', 'nid')
    ->fields('nid')
    ->condition('isbn2node_isbn_value', $isbn, '=')
    ->execute()
    ->fetchAssoc();
}


/*
 * Search for title in external database.
 *
 * @param string $title
 *   Book title. If using OpenLibrary, this can include other keywords.
 * @return object
 *   Result records from database search.
 */
function _isbn2book_search_title($title) {
  $config = config('isbn2node.settings');

  // @todo This data ought to be cached locally since ISBNdb only allows a certain number of calls each day.

  switch ($config->get('isbn2node_db')) {
    case 'isbndb':
      // Form the url.
      $url = str_replace(array('%access_key', '%title_value'),
        array($config->get('isbn2node_isbndb_access_key'), urlencode($title)),
        ISBN2NODE_ISBNDB_TITLE_URL);

      // Get and parse the XML.
      $xml = file_get_contents($url);
      $p = xml_parser_create();
      xml_parse_into_struct($p, $xml, $vals, $index);
      $vals['success'] = $vals['1']['attributes']['TOTAL_RESULTS'];
      xml_parser_free($p);
      break;

    case 'openlib':
      // Form the URL.
      $url = str_replace('%title_value',
        urlencode($title),
        ISBN2NODE_OPENLIBRARY_TITLE_URL);

      // Get and parse the Json.
      $json = file_get_contents($url);
      // _isbn2node_debug('json', $json);

      $vals = json_decode($json, False);

      // The API will return 0 as numFound if no matching records are found,
      // and the number found if any are.
      if ($vals->numFound > 0) {
        $vals->success = TRUE;
      }
      else {
        $vals->success = FALSE;
      }

      break;
  }

  return $vals;
}


/*
 * Print debugging messages.
 *
 * @param string $varname
 *   The name of the variable to be displayed, or other descriptive string.
 * @param mixed $vars
 *   The variable(s) to be displayed in the debug message.
 */
function _isbn2node_debug($varname, $vars) {
  $debug = true;
  if ($debug) {
    backdrop_set_message("<pre>DEBUG: $varname=" . print_r($vars, true) .  '</pre>');
  }
}
