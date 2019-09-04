link_protobuf_cmd='if [[ -e %s/%s/libprotobuf.so && -d %s/solib/ ]]; then cp -f %s/%s/libprotobuf.so %s/solib/; fi'%(WORK_DIR(),OUT_DIR(),WORK_DIR(),WORK_DIR(),OUT_DIR(),WORK_DIR())
cc_library(
    name = 'protobuf',
    prebuilt = True,
    deps = [
        '//mm3rd/glog:glog',
    ],
    optimize = [
		'O2',
		],
    extra_cppflags = [ 
        '-Wall',
        '-pipe',
        '-fPIC',
        '-Wno-deprecated',

	],
    export_incs = [ 'src/google/protobuf','src',],
    export_dynamic = True,
    postaction = link_protobuf_cmd,
)

