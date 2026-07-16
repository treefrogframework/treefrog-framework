from shrub.v3.evg_command import archive_targz_pack, s3_put

from config_generator.etc.function import Function


class UploadBuild(Function):
    name = 'upload-build'
    commands = [
        archive_targz_pack(
            target='${build_id}.tar.gz',
            source_dir='mongoc',
            include=['./**'],
        ),
        s3_put(
            aws_key='${aws_key}',
            aws_secret='${aws_secret}',
            remote_file='${project}/${build_variant}/${revision}/${task_name}/${build_id}.tar.gz',
            bucket='mciuploads',
            permissions='public-read',
            local_file='${build_id}.tar.gz',
            content_type='${content_type|application/x-gzip}',
        ),
    ]


def functions():
    return UploadBuild.defn()
