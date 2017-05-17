CREATE TABLE IF NOT EXISTS `pinba`.`active` (
  `id` int unsigned NOT NULL,
  `table_name` varchar(128) NOT NULL,
  `internal_name` varchar(128) NOT NULL,
  `kind` varchar(64) NOT NULL,
  `uptime` double unsigned NOT NULL,
  `time_window_sec` int(10) unsigned NOT NULL,
  `tick_count` int(10) NOT NULL,
  `approx_row_count` int(10) unsigned NOT NULL,
  `approx_mem_used` bigint(20) unsigned NOT NULL,
  `batches_sent` bigint(20) unsigned NOT NULL,
  `batches_received` bigint(20) unsigned NOT NULL,
  `packets_received` bigint(20) unsigned NOT NULL,
  `packets_lost` bigint(20) unsigned NOT NULL,
  `packets_aggregated` bigint(20) unsigned NOT NULL,
  `packets_dropped_by_bloom` bigint(20) unsigned NOT NULL,
  `packets_dropped_by_filters` bigint(20) unsigned NOT NULL,
  `packets_dropped_by_rfield` bigint(20) unsigned NOT NULL,
  `packets_dropped_by_rtag` bigint(20) unsigned NOT NULL,
  `packets_dropped_by_timertag` bigint(20) unsigned NOT NULL,
  `timers_scanned` bigint(20) unsigned NOT NULL,
  `timers_aggregated` bigint(20) unsigned NOT NULL,
  `timers_skipped_by_filters` bigint(20) unsigned NOT NULL,
  `timers_skipped_by_tags` bigint(20) unsigned NOT NULL,
  `ru_utime` double NOT NULL,
  `ru_stime` double NOT NULL,
  `last_tick_time` double NOT NULL,
  `last_tick_prepare_duration` double NOT NULL,
  `last_snapshot_merge_duration` double NOT NULL
) ENGINE=PINBA DEFAULT CHARSET=latin1 COMMENT='v2/active';