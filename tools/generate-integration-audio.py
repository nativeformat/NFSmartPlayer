import argparse
import os
import hashlib
import subprocess
from ruamel import yaml

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Generate all CI CLI')
    parser.add_argument('cli_path', help='Path to CLI')
    parser.add_argument('ci_yaml', help='Path to ci.yaml')
    parser.add_argument('tmp_path', help='Path to cli output',
                        default='/tmp/nf-int-output.wav')
    parser.add_argument('flac_dir', help='Path to flac output')
    args = parser.parse_args()

    with open(args.ci_yaml, 'r') as stream:
        cis = yaml.load(stream)

    for i in range(len(cis['integration_tests'])):
        ci = cis['integration_tests'][i]
        cli = [args.cli_path,  # Smart player
               '--input-file='+ci['graph'],
               '--driver-type=file',
               '--render-time='+str(ci['render_time']),
               '--driver-file='+args.tmp_path]

        print(cli)
        subprocess.check_call(cli)
        md5hash = hashlib.md5(open(args.tmp_path, 'rb').read()).hexdigest()
        flac_file = '{md5}.flac'.format(md5=md5hash)
        full_flac_file = os.path.join(args.flac_dir, flac_file)

        ffmpeg_cli = [
            'ffmpeg',
            '-hide_banner',
            '-loglevel',
            'panic',
            '-i',
            args.tmp_path,
            '-c:a',
            'flac',
            '-y',
            full_flac_file
        ]

        print(ffmpeg_cli)
        ffmpeg_result = subprocess.check_call(ffmpeg_cli)

        # remove tmp file because uh it helps expose errors
        os.remove(args.tmp_path)
        cis['integration_tests'][i]['audio'] = flac_file

    with open(args.ci_yaml, 'w') as outyml:
        yml = yaml.YAML()
        yml.indent(mapping=2, sequence=4, offset=2)
        yml.dump(cis, outyml)
