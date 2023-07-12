/*
 * ossfs -  FUSE-based file system backed by Alibaba Cloud OSS
 *
 * Copyright(C) 2007 Takeshi Nakatani <ggtakec.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <cstdio>
#include <cstdlib>

#include <string>

#include "common.h"
#include "s3fs.h"
#include "s3fs_help.h"
#include "s3fs_auth.h"

//-------------------------------------------------------------------
// Contents
//-------------------------------------------------------------------
static const char help_string[] = 
    "\n"
    "Mount an Alibaba Cloud OSS bucket as a file system.\n"
    "\n"
    "Usage:\n"
    "   mounting\n"
    "     ossfs bucket[:/path] mountpoint [options]\n"
    "     ossfs mountpoint [options (must specify bucket= option)]\n"
    "\n"
    "   unmounting\n"
    "     umount mountpoint\n"
    "\n"
    "   General forms for ossfs and FUSE/mount options:\n"
    "      -o opt[,opt...]\n"
    "      -o opt [-o opt] ...\n"
    "\n"
    "   utility mode (remove interrupted multipart uploading objects)\n"
    "     ossfs - -incomplete-mpu-list (-u) bucket\n"
    "     ossfs - -incomplete-mpu-abort[=all | =<date format>] bucket\n"
    "\n"
    "ossfs Options:\n"
    "\n"
    "   Most ossfs options are given in the form where \"opt\" is:\n"
    "\n"
    "             <option_name>=<option_value>\n"
    "\n"
    "   bucket\n"
    "      - if it is not specified bucket name (and path) in command line,\n"
    "        must specify this option after -o option for bucket name.\n"
    "\n"
    "   default_acl (default=\"private\")\n"
    "      - the default canned acl to apply to all written oss objects,\n"
    "        e.g., private, public-read.\n"
    "\n"
    "   retries (default=\"5\")\n"
    "      - number of times to retry a failed OSS transaction\n"
    "\n"
    "   tmpdir (default=\"/tmp\")\n"
    "      - local folder for temporary files.\n"
    "\n"
    "   use_cache (default=\"\" which means disabled)\n"
    "      - local folder to use for local file cache\n"
    "\n"
    "   check_cache_dir_exist (default is disable)\n"
    "      - if use_cache is set, check if the cache directory exists.\n"
    "        If this option is not specified, it will be created at runtime\n"
    "        when the cache directory does not exist.\n"
    "\n"
    "   del_cache (delete local file cache)\n"
    "      - delete local file cache when ossfs starts and exits.\n"
    "\n"
    "   storage_class (default=\"Standard\")\n"
    "      - store object with specified sstorage class. Possible values:\n"
    "        Standard, IA, Archive, ColdArchive, and DeepColdArchive.\n"
    "\n"
    "   use_sse (default is disable)\n"
    "      - Specify two type OSS's Server-Site Encryption: SSE-OSS\n"
    "        and SSE-KMS. SSE-OSS uses Alibaba Cloud OSS-managed encryption\n"
    "        keys and SSE-KMS uses the master key which you manage in Alibaba Cloud KMS.\n"
    "        You can specify \"use_sse\" or \"use_sse=1\" enables SSE-OSS\n"
    "        type (use_sse=1 is old type parameter).\n"
    "        For setting SSE-KMS, specify \"use_sse=kmsid\" or\n"
    "        \"use_sse=kmsid:<kms id>\". You can use \"k\" for short \"kmsid\".\n"
    "        If you san specify SSE-KMS type with your <kms id> in Alibaba Cloud\n"
    "        KMS, you can set it after \"kmsid:\" (or \"k:\"). If you\n"
    "        specify only \"kmsid\" (\"k\"), you need to set OSSSSEKMSID\n"
    "        environment which value is <kms id>. You must be careful\n"
    "        about that you can not use the KMS id which is not same EC2\n"
    "        region.\n"
    "\n"
    "   public_bucket (default=\"\" which means disabled)\n"
    "      - anonymously mount a public bucket when set to 1, ignores the \n"
    "        $HOME/.passwd-ossfs and /etc/passwd-ossfs files.\n"
    "        OSS does not allow copy object api for anonymous users, then\n"
    "        ossfs sets nocopyapi option automatically when public_bucket=1\n"
    "        option is specified.\n"
    "\n"
    "   passwd_file (default=\"\")\n"
    "      - specify which ossfs password file to use\n"
    "\n"
    "   ahbe_conf (default=\"\" which means disabled)\n"
    "      - This option specifies the configuration file path which\n"
    "      file is the additional HTTP header by file (object) extension.\n"
    "      The configuration file format is below:\n"
    "      -----------\n"
    "      line         = [file suffix or regex] HTTP-header [HTTP-values]\n"
    "      file suffix  = file (object) suffix, if this field is empty,\n"
    "                     it means \"reg:(.*)\".(=all object).\n"
    "      regex        = regular expression to match the file (object) path.\n"
    "                     this type starts with \"reg:\" prefix.\n"
    "      HTTP-header  = additional HTTP header name\n"
    "      HTTP-values  = additional HTTP header value\n"
    "      -----------\n"
    "      Sample:\n"
    "      -----------\n"
    "      .gz                    Content-Encoding  gzip\n"
    "      .Z                     Content-Encoding  compress\n"
    "      reg:^/MYDIR/(.*)[.]t2$ Content-Encoding  text2\n"
    "      -----------\n"
    "      A sample configuration file is uploaded in \"test\" directory.\n"
    "      If you specify this option for set \"Content-Encoding\" HTTP \n"
    "      header, please take care for RFC 2616.\n"
    "\n"
    "   connect_timeout (default=\"300\" seconds)\n"
    "      - time to wait for connection before giving up\n"
    "\n"
    "   readwrite_timeout (default=\"120\" seconds)\n"
    "      - time to wait between read/write activity before giving up\n"
    "\n"
    "   list_object_max_keys (default=\"1000\")\n"
    "      - specify the maximum number of keys returned by OSS list object\n"
    "        API. The default is 1000. you can set this value to 1000 or more.\n"
    "\n"
    "   max_stat_cache_size (default=\"100,000\" entries (about 40MB))\n"
    "      - maximum number of entries in the stat cache, and this maximum is\n"
    "        also treated as the number of symbolic link cache.\n"
    "\n"
    "   stat_cache_expire (default is 900))\n"
    "      - specify expire time (seconds) for entries in the stat cache.\n"
    "        This expire time indicates the time since stat cached. and this\n"
    "        is also set to the expire time of the symbolic link cache.\n"
    "\n"
    "   stat_cache_interval_expire (default is 900)\n"
    "      - specify expire time (seconds) for entries in the stat cache(and\n"
    "        symbolic link cache).\n"
    "      This expire time is based on the time from the last access time\n"
    "      of the stat cache. This option is exclusive with stat_cache_expire,\n"
    "      and is left for compatibility with older versions.\n"
    "\n"
    "   enable_noobj_cache (default is disable)\n"
    "      - enable cache entries for the object which does not exist.\n"
    "      ossfs always has to check whether file (or sub directory) exists \n"
    "      under object (path) when ossfs does some command, since ossfs has \n"
    "      recognized a directory which does not exist and has files or \n"
    "      sub directories under itself. It increases ListBucket request \n"
    "      and makes performance bad.\n"
    "      You can specify this option for performance, ossfs memorizes \n"
    "      in stat cache that the object (file or directory) does not exist.\n"
    "\n"
    "   no_check_certificate\n"
    "      - server certificate won't be checked against the available \n"
    "      certificate authorities.\n"
    "\n"
    "   ssl_verify_hostname (default=\"2\")\n"
    "      - When 0, do not verify the SSL certificate against the hostname.\n"
    "\n"
    "   nodnscache (disable DNS cache)\n"
    "      - ossfs is always using DNS cache, this option make DNS cache disable.\n"
    "\n"
    "   nosscache (disable SSL session cache)\n"
    "      - ossfs is always using SSL session cache, this option make SSL \n"
    "      session cache disable.\n"
    "\n"
    "   multireq_max (default=\"20\")\n"
    "      - maximum number of parallel request for listing objects.\n"
    "\n"
    "   parallel_count (default=\"5\")\n"
    "      - number of parallel request for uploading big objects.\n"
    "      ossfs uploads large object (over 20MB) by multipart post request, \n"
    "      and sends parallel requests.\n"
    "      This option limits parallel request count which ossfs requests \n"
    "      at once. It is necessary to set this value depending on a CPU \n"
    "      and a network band.\n"
    "\n"
    "   multipart_size (default=\"10\")\n"
    "      - part size, in MB, for each multipart request.\n"
    "      The minimum value is 5 MB and the maximum value is 5 GB.\n"
    "\n"
    "   multipart_copy_size (default=\"512\")\n"
    "      - part size, in MB, for each multipart copy request, used for\n"
    "      renames and mixupload.\n"
    "      The minimum value is 5 MB and the maximum value is 5 GB.\n"
    "      Must be at least 512 MB to copy the maximum 5 TB object size\n"
    "      but lower values may improve performance.\n"
    "\n"
    "   max_dirty_data (default=\"5120\")\n"
    "      - flush dirty data to OSS after a certain number of MB written.\n"
    "      The minimum value is 50 MB. -1 value means disable.\n"
    "      Cannot be used with nomixupload.\n"
    "\n"
    "   ensure_diskfree (default 0)\n"
    "      - sets MB to ensure disk free space. This option means the\n"
    "        threshold of free space size on disk which is used for the\n"
    "        cache file by ossfs. ossfs makes file for\n"
    "        downloading, uploading and caching files. If the disk free\n"
    "        space is smaller than this value, ossfs do not use disk space\n"
    "        as possible in exchange for the performance.\n"
    "\n"
    "   multipart_threshold (default=\"25\")\n"
    "      - threshold, in MB, to use multipart upload instead of\n"
    "        single-part. Must be at least 5 MB.\n"
    "\n"
    "   singlepart_copy_limit (default=\"512\")\n"
    "      - maximum size, in MB, of a single-part copy before trying \n"
    "      multipart copy.\n"
    "\n"
    "   host (default=\"https://oss-cn-hangzhou.aliyuncs.com\")\n"
    "      - Set a host, e.g., https://example.com.\n"
    "\n"
    "   servicepath (default=\"/\")\n"
    "      - Set a service path when the host requires a prefix.\n"
    "\n"
    "   url (default=\"https://oss-cn-hangzhou.aliyuncs.com\")\n"
    "      - sets the url to use to access Alibaba Cloud OSS. If you want to use HTTP,\n"
    "        then you can set \"url=http://oss-cn-hangzhou.aliyuncs.com\".\n"
    "        If you do not use https, please specify the URL with the url\n"
    "        option.\n"
    "\n"
    "   endpoint (default=\"us-east-1\")\n"
    "      - sets the endpoint to use on signature version 4\n"
    "      If this option is not specified, ossfs uses \"us-east-1\" region as\n"
    "      the default. If the ossfs could not connect to the region specified\n"
    "      by this option, ossfs could not run. But if you do not specify this\n"
    "      option, and if you can not connect with the default region, ossfs\n"
    "      will retry to automatically connect to the other region. So ossfs\n"
    "      can know the correct region name, because ossfs can find it in an\n"
    "      error from the oss server.\n"
    "\n"
    "   sigv1 (default is signature version 1)\n"
    "      - sets signing OSS requests by using only signature version 1\n"
    "\n"
    "   mp_umask (default is \"0000\")\n"
    "      - sets umask for the mount point directory.\n"
    "      If allow_other option is not set, ossfs allows access to the mount\n"
    "      point only to the owner. In the opposite case ossfs allows access\n"
    "      to all users as the default. But if you set the allow_other with\n"
    "      this option, you can control the permissions of the\n"
    "      mount point by this option like umask.\n"
    "\n"
    "   umask (default is \"0000\")\n"
    "      - sets umask for files under the mountpoint. This can allow\n"
    "      users other than the mounting user to read and write to files\n"
    "      that they did not create.\n"
    "\n"
    "   nomultipart (disable multipart uploads)\n"
    "\n"
    "   enable_content_md5 (default is disable)\n"
    "      - Allow OSS server to check data integrity of uploads via the\n"
    "      Content-MD5 header. This can add CPU overhead to transfers.\n"
    "\n"
    "   enable_unsigned_payload (default is disable)\n"
    "      - Do not calculate Content-SHA256 for PutObject and UploadPart\n"
    "      payloads. This can reduce CPU overhead to transfers.\n"
    "\n"
    "   ram_role (default is no IAM role)\n"
    "      - This option requires the IAM role name or \"auto\". If you specify\n"
    "      \"auto\", ossfs will automatically use the IAM role names that are set\n"
    "      to an instance. If you specify this option without any argument, it\n"
    "      is the same as that you have specified the \"auto\".\n"
    "\n"
    "   use_xattr (default is not handling the extended attribute)\n"
    "      Enable to handle the extended attribute (xattrs).\n"
    "      If you set this option, you can use the extended attribute.\n"
    "      For example, encfs and ecryptfs need to support the extended attribute.\n"
    "      Notice: if ossfs handles the extended attribute, ossfs can not work to\n"
    "      copy command with preserve=mode.\n"
    "\n"
    "   noxmlns (disable registering xml name space)\n"
    "        disable registering xml name space for response of \n"
    "        ListBucketResult and ListVersionsResult etc.\n"
    "        This option should not be specified now, because ossfs looks up\n"
    "        xmlns automatically after v1.66.\n"
    "\n"
    "   nomixupload (disable copy in multipart uploads)\n"
    "        Disable to use PUT (copy api) when multipart uploading large size objects.\n"
    "        By default, when doing multipart upload, the range of unchanged data\n"
    "        will use PUT (copy api) whenever possible.\n"
    "        When nocopyapi or norenameapi is specified, use of PUT (copy api) is\n"
    "        invalidated even if this option is not specified.\n"
    "\n"
    "   nocopyapi (for other incomplete compatibility object storage)\n"
    "        Enable compatibility with APIs which do not support\n"
    "        PUT (copy api).\n"
    "        If you set this option, ossfs do not use PUT with \n"
    "        \"x-oss-copy-source\" (copy api). Because traffic is increased\n"
    "        2-3 times by this option, we do not recommend this.\n"
    "\n"
    "   norenameapi (for other incomplete compatibility object storage)\n"
    "        Enable compatibility with APIs which do not support\n"
    "        PUT (copy api).\n"
    "        This option is a subset of nocopyapi option. The nocopyapi\n"
    "        option does not use copy-api for all command (ex. chmod, chown,\n"
    "        touch, mv, etc), but this option does not use copy-api for\n"
    "        only rename command (ex. mv). If this option is specified with\n"
    "        nocopyapi, then ossfs ignores it.\n"
    "\n"
    "   use_path_request_style (use legacy API calling style)\n"
    "        Enable compatibility with APIs which do not support\n"
    "        the virtual-host request style, by using the older path request\n"
    "        style.\n"
    "\n"
    "   listobjectsv2 (use ListObjectsV2)\n"
    "        Issue ListObjectsV2 instead of ListObjects, useful on object\n"
    "        stores without ListObjects support.\n"
    "\n"
    "   noua (suppress User-Agent header)\n"
    "        Usually ossfs outputs of the User-Agent in \"ossfs/<version> (commit\n"
    "        hash <hash>; <using ssl library name>)\" format.\n"
    "        If this option is specified, ossfs suppresses the output of the\n"
    "        User-Agent.\n"
    "\n"
    "   cipher_suites\n"
    "        Customize the list of TLS cipher suites.\n"
    "        Expects a colon separated list of cipher suite names.\n"
    "        A list of available cipher suites, depending on your TLS engine,\n"
    "        can be found on the CURL library documentation:\n"
    "        https://curl.haxx.se/docs/ssl-ciphers.html\n"
    "\n"
    "   instance_name - The instance name of the current ossfs mountpoint.\n"
    "        This name will be added to logging messages and user agent headers sent by ossfs.\n"
    "\n"
    "   complement_stat (complement lack of file/directory mode)\n"
    "        ossfs complements lack of information about file/directory mode\n"
    "        if a file or a directory object does not have x-oss-meta-mode\n"
    "        header. As default, ossfs does not complements stat information\n"
    "        for a object, then the object will not be able to be allowed to\n"
    "        list/modify.\n"
    "\n"
    "   notsup_compat_dir (disable support of alternative directory names)\n"
    "        ossfs supports the three different naming schemas \"dir/\",\n"
    "        \"dir\" and \"dir_$folder$\" to map directory names to OSS\n"
    "        objects and vice versa. As a fourth variant, directories can be\n"
    "        determined indirectly if there is a file object with a path (e.g.\n"
    "        \"/dir/file\") but without the parent directory.\n"
    "        \n"
    "        ossfs uses only the first schema \"dir/\" to create OSS objects for\n"
    "        directories."
    "        \n"
    "        The support for these different naming schemas causes an increased\n"
    "        communication effort.\n"
    "        \n"
    "        If all applications exclusively use the \"dir/\" naming scheme and\n"
    "        the bucket does not contain any objects with a different naming \n"
    "        scheme, this option can be used to disable support for alternative\n"
    "        naming schemes. This reduces access time and can save costs.\n"
    "\n"
    "   use_wtf8 - support arbitrary file system encoding.\n"
    "        OSS requires all object names to be valid UTF-8. But some\n"
    "        clients, notably Windows NFS clients, use their own encoding.\n"
    "        This option re-encodes invalid UTF-8 object names into valid\n"
    "        UTF-8 by mapping offending codes into a 'private' codepage of the\n"
    "        Unicode set.\n"
    "        Useful on clients not using UTF-8 as their file system encoding.\n"
    "\n"
    "   use_session_token - indicate that session token should be provided.\n"
    "        If credentials are provided by environment variables this switch\n"
    "        forces presence check of OSSSESSIONTOKEN variable.\n"
    "        Otherwise an error is returned.\n"
    "\n"
    "   requester_pays (default is disable)\n"
    "        This option instructs ossfs to enable requests involving\n"
    "        Requester Pays buckets.\n"
    "        It includes the 'x-oss-request-payer=requester' entry in the\n"
    "        request header.\n"
    "\n"
    "   mime (default is \"/etc/mime.types\")\n"
    "        Specify the path of the mime.types file.\n"
    "        If this option is not specified, the existence of \"/etc/mime.types\"\n"
    "        is checked, and that file is loaded as mime information.\n"
    "        If this file does not exist on macOS, then \"/etc/apache2/mime.types\"\n"
    "        is checked as well.\n"
    "\n"
    "   upload_traffic_limit (default=\"0\")\n"
    "        The single-connection bandwidth throttling, in bit/s, for each upload request.\n"
    "        The minimum value is 819200 bit/s and the maximum value is 838860800 bit/s.\n"
    "        0 value means disable.\n"
    "\n"
    "   download_traffic_limit (default=\"0\")\n"
    "        The single-connection bandwidth throttling, in bit/s, for each download request.\n"
    "        The minimum value is 819200 bit/s and the maximum value is 838860800 bit/s.\n"
    "        0 value means disable.\n"
    "\n"
    "   noshallowcopyapi - disable using shallow copy feature\n"
    "        OSS supports shallow copy feature, so try using the single-part copy api for a large file renaming\n"
    "        or a large file's meta updating. If that fails, use the multipart copy api again.\n"
    "        It can speed up the process of renaming a large file or updating a file's meta.\n"
    "        If this option is specified, renames a large file or update a large file's meta by multipart upload.\n"
    "\n"
    "   readdir_optimize (default is disable)\n"
    "        ossfs stroes extended information like access & change time, owner, permissions, etc in the object's meta data.\n"
    "        The ListObjects only returns basic information like name, size, and modified time, so ossfs issues\n"
    "        HeadObject request for each file to get extended information. This causes poor performance in readdir.\n"
    "        If this option is specified, ossfs improves readdir preformence by ignoring extended information and\n"
    "        faking these metadata - atime/ctime, uid/gid, and permissions.\n"
    "        If this option is specified, chmod and chown works nothing.\n"
    "        If this option is specified, only the symlink built in new symlink format can be displayed correctly.\n"
    "\n"
    "   readdir_check_size (default=\"0\")\n"
    "        maximum size in file length of issuing HeadObject request to get extended information.\n"
    "        If a file size is greater than this threshold, only use the basic information from ListObjects result.\n"
    "        This option works when readdir_optimize option is eanbled.\n"
    "\n"
    "   symlink_in_meta (default is disable)\n"
    "        Enable to save the symbolic link target in object user metadata.\n"
    "        This option is used in conjunction with the readdir_optimize option.\n"
    "\n"
    "   logfile - specify the log output file.\n"
    "        ossfs outputs the log file to syslog. Alternatively, if ossfs is\n"
    "        started with the \"-f\" option specified, the log will be output\n"
    "        to the stdout/stderr.\n"
    "        You can use this option to specify the log file that ossfs outputs.\n"
    "        If you specify a log file with this option, it will reopen the log\n"
    "        file when ossfs receives a SIGHUP signal. You can use the SIGHUP\n"
    "        signal for log rotation.\n"
    "\n"
    "   dbglevel (default=\"crit\")\n"
    "        Set the debug message level. set value as crit (critical), err\n"
    "        (error), warn (warning), info (information) to debug level.\n"
    "        default debug level is critical. If ossfs run with \"-d\" option,\n"
    "        the debug level is set information. When ossfs catch the signal\n"
    "        SIGUSR2, the debug level is bump up.\n"
    "\n"
    "   curldbg - put curl debug message\n"
    "        Put the debug message from libcurl when this option is specified.\n"
    "        Specify \"normal\" or \"body\" for the parameter.\n"
    "        If the parameter is omitted, it is the same as \"normal\".\n"
    "        If \"body\" is specified, some API communication body data will be\n"
    "        output in addition to the debug message output as \"normal\".\n"
    "\n"
    "   no_time_stamp_msg - no time stamp in debug message\n"
    "        The time stamp is output to the debug message by default.\n"
    "        If this option is specified, the time stamp will not be output\n"
    "        in the debug message.\n"
    "        It is the same even if the environment variable \"OSSFS_MSGTIMESTAMP\"\n"
    "        is set to \"no\".\n"
    "\n"
    "   set_check_cache_sigusr1 (default is stdout)\n"
    "        If the cache is enabled, you can check the integrity of the\n"
    "        cache file and the cache file's stats info file.\n"
    "        This option is specified and when sending the SIGUSR1 signal\n"
    "        to the ossfs process checks the cache status at that time.\n"
    "        This option can take a file path as parameter to output the\n"
    "        check result to that file. The file path parameter can be omitted.\n"
    "        If omitted, the result will be output to stdout or syslog.\n"
    "\n"
    "FUSE/mount Options:\n"
    "\n"
    "   Most of the generic mount options described in 'man mount' are\n"
    "   supported (ro, rw, suid, nosuid, dev, nodev, exec, noexec, atime,\n"
    "   noatime, sync async, dirsync). Filesystems are mounted with\n"
    "   '-onodev,nosuid' by default, which can only be overridden by a\n"
    "   privileged user.\n"
    "   \n"
    "   There are many FUSE specific mount options that can be specified.\n"
    "   e.g. allow_other  See the FUSE's README for the full set.\n"
    "\n"
    "Utility mode Options:\n"
    "\n"
    " -u, --incomplete-mpu-list\n"
    "        Lists multipart incomplete objects uploaded to the specified\n"
    "        bucket.\n"
    " --incomplete-mpu-abort (=all or =<date format>)\n"
    "        Delete the multipart incomplete object uploaded to the specified\n"
    "        bucket.\n"
    "        If \"all\" is specified for this option, all multipart incomplete\n"
    "        objects will be deleted. If you specify no argument as an option,\n"
    "        objects older than 24 hours (24H) will be deleted (This is the\n"
    "        default value). You can specify an optional date format. It can\n"
    "        be specified as year, month, day, hour, minute, second, and it is\n"
    "        expressed as \"Y\", \"M\", \"D\", \"h\", \"m\", \"s\" respectively.\n"
    "        For example, \"1Y6M10D12h30m30s\".\n"
    "\n"
    "Miscellaneous Options:\n"
    "\n"
    " -h, --help        Output this help.\n"
    "     --version     Output version info.\n"
    " -d  --debug       Turn on DEBUG messages to syslog. Specifying -d\n"
    "                   twice turns on FUSE debug messages to STDOUT.\n"
    " -f                FUSE foreground option - do not run as daemon.\n"
    " -s                FUSE single-threaded option\n"
    "                   disable multi-threaded operation\n"
    "\n"
    "\n"
    "ossfs home page: <https://github.com/aliyun/ossfs>\n"
    ;

//-------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------
void show_usage()
{
    printf("Usage: %s BUCKET:[PATH] MOUNTPOINT [OPTION]...\n", program_name.c_str());
}

void show_help()
{
    show_usage();
    printf(help_string);
}

void show_version()
{
    printf(
    "Alibaba Cloud Open Storage Service File System V%s (commit:%s) with %s\n"
    "Copyright (C) 2010 Randy Rizun <rrizun@gmail.com>\n"
    "License GPL2: GNU GPL version 2 <https://gnu.org/licenses/gpl.html>\n"
    "This is free software: you are free to change and redistribute it.\n"
    "There is NO WARRANTY, to the extent permitted by law.\n",
    VERSION, COMMIT_HASH_VAL, s3fs_crypt_lib_name());
}

const char* short_version()
{
    static const char short_ver[] = "ossfs version " VERSION "(" COMMIT_HASH_VAL ")";
    return short_ver;
}

/*
* Local variables:
* tab-width: 4
* c-basic-offset: 4
* End:
* vim600: expandtab sw=4 ts=4 fdm=marker
* vim<600: expandtab sw=4 ts=4
*/
