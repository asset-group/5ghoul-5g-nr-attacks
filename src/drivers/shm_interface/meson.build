libshm_interface_sources = files('''
	wd_shm.c
	wd_shm.h
	wd_shm_nr_utils.h
	wd_shm_nas_utils.h
	wd_shm_lte_utils.h
'''.split())

libshm_interface_inc = include_directories('.')

libpthread_dep    = cc.find_library('libpthread')
libz_dep          = cc.find_library('libz')
libm_dep          = cc.find_library('libm')
librt_dep         = cc.find_library('librt')

libshm_interface = static_library('shm_interface', 
    sources : libshm_interface_sources,
    include_directories : libshm_interface_inc,
    dependencies : [libpthread_dep,
                    libz_dep,
                    libm_dep,
                    librt_dep])
libshm_interface_dep = declare_dependency(
    link_with : libshm_interface,
    include_directories : libshm_interface_inc,
    dependencies : [libpthread_dep,
                    libz_dep,
                    libm_dep,
                    librt_dep])
