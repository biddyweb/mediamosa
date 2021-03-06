<?php
/**
 * @file
 * Wrapper around Apache_Solr_Response.
 */

class mediamosa_solr_apache_solr_response {
  /**
   * The decoded json response.
   *
   * @var object
   */
  private $_response = NULL;

  // -------------------------------------------------------------- Constructor.
  /**
   * Constructor.
   */
  public function __construct(Apache_Solr_Response $response) {
    $this->_response = json_decode($response->getRawResponse());
  }

  // ---------------------------------------------------------------- Functions.
  /**
   * Get the decoded json response.
   *
   * @return object
   *   The decoded json response.
   */
  public function getResponse() {
    return $this->_response;
  }

  /**
   * Get total found rows.
   *
   * This is not the number of found rows, but total matched to
   * the query.
   *
   * @return int
   *   The number of found rows.
   */
  public function getNumFound() {
    return empty($this->_response->response->numFound) ? 0 : (int) $this->_response->response->numFound;
  }

  /**
   * Get the asset ids.
   *
   * @return array
   *   Array of found asset IDs.
   */
  public function getAssetIds() {
    $asset_ids = array();

    if (!empty($this->_response->response->docs)) {
      foreach ($this->_response->response->docs as $doc) {
        $asset_ids[] = (string) $doc->asset_id;
      }
    }

    return $asset_ids;
  }

  /**
   * Retrieve the schema name.
   *
   * Is only available for Solr 4.2 and higher and using the /schema/name url.
   *
   * @return string
   *   The schema name.
   */
  public function getSchemaName() {
    return empty($this->_response->name) ? 'No schema found' : $this->_response->name;
  }

  /**
   * Convert the metadata field name from solr to MM field.
   *
   * @param string $solrfield
   *   The Solr field to convert.
   *
   * @return string
   *   The solr field.
   */
  protected function solrfield2metadatafield($solrfield) {
    return $solrfield;
  }

  /**
   * Get facets.
   *
   * In output from Solr we ignore;
   * - 'facet_query': Requires solr query, needs work (CQL -> solr).
   * - 'facet_range': Requires complex field specified input.
   *
   * @return array
   *   Related information. Key contains asset_id.
   *   - 'score': The score related.
   */
  public function getFacets() {
    $facets = array('solr_facet_fields' => array(), 'solr_facet_dates' => array());

    // facet_fields;
    // granted=true & calculate_total_count=true & solr_facet=true
    // & solr_facet_field=title_vc

    // facet_dates;
    // granted=true & calculate_total_count=true & solr_facet=true &
    // solr_facet_date=videotimestampmodified &
    // solr_facet_date_start=NOW/DAY-5DAYS & solr_facet_date_end=NOW/DAY%2B1DAY
    // & solr_facet_date_gap=%2B1DAY

    if (!empty($this->_response->facet_counts)) {
      if (!empty($this->_response->facet_counts->facet_fields)) {
        foreach ($this->_response->facet_counts->facet_fields as $field => $data) {
          if (mediamosa_unicode::substr($field, -3) == '_md') {
            $field = mediamosa_unicode::substr($field, 0, -3);
          }
          $facets['solr_facet_fields'][$field] = array();
          foreach ($data as $name => $value) {
            $facets['solr_facet_fields'][$field][$name] = (string) $value;
          }
        }

        foreach ($this->_response->facet_counts->facet_dates as $field => $dates) {
          if (mediamosa_unicode::substr($field, -3) == '_md') {
            $field = mediamosa_unicode::substr($field, 0, -3);
            $facets['solr_facet_dates'][$field] = array();
            foreach ($dates as $name => $value) {
              $name = mediamosa_unicode::substr($name, 0, -3);
              $facets['solr_facet_dates'][$field][$name] = (string) $value;
            }
          }
        }
      }
    }

    return $facets;
  }

  /**
   * Get related.
   *
   * @return array
   *   Related information. Key contains asset_id.
   *   - 'score': The score related.
   */
  public function getRelated() {
    $related = array();

    if (!empty($this->_response->response)) {
      $assets = array();
      foreach ($this->_response->response->docs as $doc) {
        $assets[$doc->asset_id] = array('score' => $doc->score, 'asset_id' => $doc->asset_id);
        foreach ($doc as $name => $value) {
          if (mediamosa_unicode::substr($name, -3) == '_md') {
            $name = mediamosa_unicode::substr($name, 0, -3);

            if (substr($name, 0, 4) !== mediamosa_asset_metadata_property_group::APP_PREFIX) {
              list($group_name_short, $name) = explode('_', $name, 2);
            }
            else {
              list($group_name_short, $app_id, $name) = explode('_', $name, 3);
              $group_name_short .= '_' . $app_id;
            }

            $context_full = mediamosa_asset_metadata_property_group::property_group_get_context_full($group_name_short);
            $assets[$doc->asset_id]['metadata'][$context_full][$name] = $value;
          }
        }
      }

      $related = array(
        'count_total' => (int) $this->_response->response->numFound,
        'count' => count($assets),
        'offset' => (int) $this->_response->response->start,
        'score_max' => (string) $this->_response->response->maxScore,
        'assets' => $assets,
      );
    }

    return $related;
  }
}
