# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging
import json
import os

from telemetry import facebook_credentials_backend
from telemetry import google_credentials_backend
from telemetry import options_for_unittests

class BrowserCredentials(object):
  def __init__(self, backends = None):
    self._credentials = {}
    self._credentials_path = None
    self._extra_credentials = {}

    if backends is None:
      backends = [
        facebook_credentials_backend.FacebookCredentialsBackend(),
        google_credentials_backend.GoogleCredentialsBackend()]

    self._backends = {}
    for backend in backends:
      self._backends[backend.credentials_type] = backend

  def AddBackend(self, backend):
    assert backend.credentials_type not in self._backends
    self._backends[backend.credentials_type] = backend

  def CanLogin(self, credentials_type):
    if credentials_type not in self._backends:
      raise Exception('Unrecognized credentials type: %s', credentials_type)
    return credentials_type in self._credentials

  def LoginNeeded(self, tab, credentials_type):
    if credentials_type not in self._backends:
      raise Exception('Unrecognized credentials type: %s', credentials_type)
    if credentials_type not in self._credentials:
      return False
    return self._backends[credentials_type].LoginNeeded(
      tab, self._credentials[credentials_type])

  def LoginNoLongerNeeded(self, tab, credentials_type):
    assert credentials_type in self._backends
    self._backends[credentials_type].LoginNoLongerNeeded(tab)

  @property
  def credentials_path(self):
    return self._credentials_path

  @credentials_path.setter
  def credentials_path(self, credentials_path):
    self._credentials_path = credentials_path
    self._RebuildCredentials()

  def Add(self, credentials_type, data):
    if credentials_type not in self._extra_credentials:
      self._extra_credentials[credentials_type] = {}
    for k, v in data.items():
      assert k not in self._extra_credentials[credentials_type]
      self._extra_credentials[credentials_type][k] = v
    self._RebuildCredentials()

  def _RebuildCredentials(self):
    credentials = {}
    if self._credentials_path == None:
      pass
    elif os.path.exists(self._credentials_path):
      with open(self._credentials_path, 'r') as f:
        credentials = json.loads(f.read())

    # TODO(nduca): use system keychain, if possible.
    homedir_credentials_path = os.path.expanduser('~/.telemetry-credentials')
    homedir_credentials = {}

    if (not options_for_unittests.GetCopy() and
        os.path.exists(homedir_credentials_path)):
      logging.info("Found ~/.telemetry-credentials. Its contents will be used "
                   "when no other credentials can be found.")
      with open(homedir_credentials_path, 'r') as f:
        homedir_credentials = json.loads(f.read())

    self._credentials = {}
    all_keys = set(credentials.keys()).union(
      homedir_credentials.keys()).union(
      self._extra_credentials.keys())

    for k in all_keys:
      if k in credentials:
        self._credentials[k] = credentials[k]
      if k in homedir_credentials:
        logging.info("Will use ~/.telemetry-credentials for %s logins." % k)
        self._credentials[k] = homedir_credentials[k]
      if k in self._extra_credentials:
        self._credentials[k] = self._extra_credentials[k]

  def WarnIfMissingCredentials(self, page_set):
    num_pages_missing_login = 0
    missing_credentials = set()
    for page in page_set:
      if (page.credentials
          and not self.CanLogin(page.credentials)):
        num_pages_missing_login += 1
        missing_credentials.add(page.credentials)

    if num_pages_missing_login > 0:
      files_to_tweak = []
      if page_set.credentials_path:
        files_to_tweak.append(
          os.path.relpath(os.path.join(page_set.base_dir,
                                       page_set.credentials_path)))
      files_to_tweak.append('~/.telemetry-credentials')

      example_credentials_file = (
        os.path.relpath(
          os.path.join(
            os.path.dirname(__file__),
            '..', 'examples', 'credentials_example.json')))

      logging.warning("""
        Credentials for %s were not found. %i pages will not be benchmarked.

        To fix this, either add svn-internal to your .gclient using
        http://goto/read-src-internal, or add your own credentials to:
            %s
        An example credentials file you can copy from is here:
            %s\n""" % (', '.join(missing_credentials),
         num_pages_missing_login,
         ' or '.join(files_to_tweak),
         example_credentials_file))
