#!/usr/bin/env python3

import subprocess
import urllib.request
import json
from struct import unpack
from time import sleep

def uint64_to_string(x):
  return '{:0>16}'.format(hex(unpack('<Q', x)[0]).lstrip('0x'))

def run_test():
  N = 10000
  testsetFilename = '../../../../pwned-lib/test/testset-{}-existent-collection1+2+3+4+5.md5'.format(N)
  webservice = subprocess.Popen([
    '../../pwned-server/pwned-server',
    '-I', testsetFilename,
    '-W', '16',
    '-T', '2'
  ])
  sleep(1)
  rc = 0
  with open(testsetFilename, 'rb') as f:
    found = 0
    while True:
      upper = f.read(8)
      if not upper:
        break
      lower = f.read(8)
      hash = uint64_to_string(upper) + uint64_to_string(lower)
      count, = unpack('<i', f.read(4))
      try:
        url_ctx = urllib.request.urlopen('http://localhost:31337/v1/pwned/api/lookup?hash=' + hash)
      except urllib.error.URLError as err:
        print(err)
        rc = 1
        break
      if url_ctx.getcode() != 200:
        print('Invalid HTTP response code: {} (should be 200)'.format(url_ctx.getcode()))
        rc = 1
        break
      response = url_ctx.read().decode('utf-8')
      url_ctx.close()
      data = None
      try:
        data = json.loads(response)
      except json.decoder.JSONDecodeError as err:
        print(err)
        rc = 1
        break
      if not isinstance(data, dict) \
          or 'lookup-time-ms' not in data \
          or not isinstance(data['lookup-time-ms'], float) \
          or 'found' not in data \
          or not isinstance(data['found'], int) \
          or 'hash' not in data \
          or not isinstance(data['hash'], str):
        rc = 1
        break
      if (data['hash'] == hash and data['found'] == count):
        found += 1
    if found != N:
      rc = 1

  webservice.kill()
  webservice.wait()
  return rc

if __name__ == '__main__':
  rc = run_test()
  exit(rc)
