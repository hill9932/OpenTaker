storage = {
	path = "/share/OpenTaker/debug/storage1|2",  -- GB
	file_size = 512,	-- the file size saved on disk, default is 512 MB, should >= 2
}

engine = {
	enable_n2disk = true,					-- enable to store packets data to disk, default is true
	enable_parse_packet = true,
	block_share_memory_size = 10,			-- the size of share memory to keep the packets data, default is 100 MB
	debug_mode = 1,							-- 1: record block information, 2: record packet information, default is 0
}

THRIFT = {
	listen_port = 9010
}

postgreSQL = {
	connection = "localhost",
    port=5432,
	user = "postgres",
	password = "postgres",
	db = "pvc_pktagent",

    pool_size = 4,
}





