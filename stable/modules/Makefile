include ../Make.properties

MODS = billing refresh negative_ttl header_three errorpage tos url_simplify statistic header offline httpversion header_combination connection_limit cc_fast use_server_date modify_s2o_header_name ms_client_ip refresh_pattern individual_limit_speed receipt customized_server_side_error_page merge_header limit_obj_count flv url_rewrite storedir_max_load do_not_unlink mp4 store_multi_url disable_sendfile polityword_filter show_ip trend_if_none_match f4v ctrl_f5_refresh acl_optimize client_304_expires helper_defer ccv client_keepalive cookie_count no_cache_if_http_code lastmodified_verify trend_receipt partition_store active_compress debug_log xtproxy cacheable_if_reply_header access_freq_limit_for_singleIP redirect_location disk_cleanup multisquid_lscs netease_phone_download_auth cookie_store_url Powered-By-ChinaCache_header_details pps append_parameter limit_speed_by_url_param f4v_byte_13_miss_wait url_checksign setcookie oupeng ninety_nine url_range content_length_warning zone apple_receipt lenovo_download_auth token_auth merge_request store_url_rewrite pplive_vod pplive server_persist_connections persist_request_timeout post_persistent hdr_store_multi cpis_dns_modify polling_ServerIP cpis_body_modify cpis_iku video_header use_script ca dst_ip hotspot  modify_http_version zone_ip2location disable_vary no_cache_if_no_reply_header no_cache_if_reply_header http_protocol_modify coss_membuf range elong_cookie assign_dir modify_s2c_header check_response async_load push_to_preload m3u8_prefetch access_filter client_compress offline_by_code httpAclExtend async_preload_mop cpis_access_control cpis_hotspot select_disk automatic_switch ims_check prevent_tamper_for_msn

all: clean
	for i in $(MODS); do make -C $$i || exit 1; done
install-rpm:
	rm -rf $(MOD_DIR);mkdir $(MOD_DIR);
	for i in $(MODS); do cd $$i;make install;cd ..; done
	cp -rf $(MOD_DIR) $(DESTDIR);
	rm -rf $(MOD_DIR);
install-tar: 
	rm -rf $(MOD_DIR);mkdir $(MOD_DIR);
	for i in $(MODS); do cd $$i;make install;cd ..; done
	cp -rf $(MOD_DIR) ../$(SQUID_DIR)/sbin/;
	rm -rf $(MOD_DIR);
clean:
	for i in $(MODS); do cd $$i;make clean;cd ..; done
