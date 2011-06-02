<?xml version='1.0' encoding='ISO-8859-1' standalone='yes' ?>
<tagfile>
  <compound kind="file">
    <name>dvdid.h</name>
    <path>/home/chris/wrk/.medialibrary-tags/dvdid-0.1.1/include/dvdid/</path>
    <filename>dvdid_8h</filename>
    <includes id="export_8h" name="export.h" local="yes" imported="no">export.h</includes>
    <member kind="typedef">
      <type>enum dvdid_status_e</type>
      <name>dvdid_status_t</name>
      <anchorfile>dvdid_8h.html</anchorfile>
      <anchor>347935550067ecea8b82ecb687cd0179</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <name>dvdid_status_e</name>
      <anchorfile>dvdid_8h.html</anchorfile>
      <anchor>c939837112ea60d7131a2e36870dc649</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DVDID_STATUS_OK</name>
      <anchorfile>dvdid_8h.html</anchorfile>
      <anchor>c939837112ea60d7131a2e36870dc6494d8d0bf35be36c89dfe36ea9ce8be745</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DVDID_STATUS_MALLOC_ERROR</name>
      <anchorfile>dvdid_8h.html</anchorfile>
      <anchor>c939837112ea60d7131a2e36870dc6493017c7d3f2aefd593204806a704dac39</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DVDID_STATUS_PLATFORM_UNSUPPORTED</name>
      <anchorfile>dvdid_8h.html</anchorfile>
      <anchor>c939837112ea60d7131a2e36870dc64979e4d3c2087687df9cea521f7c534f08</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DVDID_STATUS_READ_VIDEO_TS_ERROR</name>
      <anchorfile>dvdid_8h.html</anchorfile>
      <anchor>c939837112ea60d7131a2e36870dc649fae6f33ad7a4d4364384951e7a7481e1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DVDID_STATUS_READ_VMGI_ERROR</name>
      <anchorfile>dvdid_8h.html</anchorfile>
      <anchor>c939837112ea60d7131a2e36870dc649000bc5247776d9a6a80a069c7b5a28b6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DVDID_STATUS_READ_VTS01I_ERROR</name>
      <anchorfile>dvdid_8h.html</anchorfile>
      <anchor>c939837112ea60d7131a2e36870dc6494ecf2f6e9dc97eee9f91c4b9d12491d1</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>dvdid_status_t</type>
      <name>dvdid_calculate</name>
      <anchorfile>dvdid_8h.html</anchorfile>
      <anchor>d8341920addbf2961084115b293ac988</anchor>
      <arglist>(uint64_t *dvdid, const char *dvdpath, int *errn)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>dvdid2.h</name>
    <path>/home/chris/wrk/.medialibrary-tags/dvdid-0.1.1/include/dvdid/</path>
    <filename>dvdid2_8h</filename>
    <includes id="export_8h" name="export.h" local="yes" imported="no">export.h</includes>
    <includes id="dvdid_8h" name="dvdid.h" local="yes" imported="no">dvdid.h</includes>
    <class kind="struct">dvdid_fileinfo_s</class>
    <member kind="define">
      <type>#define</type>
      <name>DVDID_HASHINFO_VXXI_MAXBUF</name>
      <anchorfile>dvdid2_8h.html</anchorfile>
      <anchor>f5d284b35d0ca7d97c37f259e3114db0</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct dvdid_hashinfo_s</type>
      <name>dvdid_hashinfo_t</name>
      <anchorfile>dvdid2_8h.html</anchorfile>
      <anchor>dac97eaa1954c046664231bd9553c421</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>struct dvdid_fileinfo_s</type>
      <name>dvdid_fileinfo_t</name>
      <anchorfile>dvdid2_8h.html</anchorfile>
      <anchor>1bd85094147d262980816a3992160664</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>dvdid_status_t</type>
      <name>dvdid_calculate2</name>
      <anchorfile>dvdid2_8h.html</anchorfile>
      <anchor>df1929ea584816cf95a5abcb761bc437</anchor>
      <arglist>(uint64_t *dvdid, const dvdid_hashinfo_t *hi)</arglist>
    </member>
    <member kind="function">
      <type>dvdid_status_t</type>
      <name>dvdid_hashinfo_create</name>
      <anchorfile>dvdid2_8h.html</anchorfile>
      <anchor>70d33c48f4b5def7cf99579e425afee7</anchor>
      <arglist>(dvdid_hashinfo_t **hi)</arglist>
    </member>
    <member kind="function">
      <type>dvdid_status_t</type>
      <name>dvdid_hashinfo_addfile</name>
      <anchorfile>dvdid2_8h.html</anchorfile>
      <anchor>ebac9b0738ae9cb7a1d728d6b5f0f276</anchor>
      <arglist>(dvdid_hashinfo_t *hi, const dvdid_fileinfo_t *fi)</arglist>
    </member>
    <member kind="function">
      <type>dvdid_status_t</type>
      <name>dvdid_hashinfo_set_vmgi</name>
      <anchorfile>dvdid2_8h.html</anchorfile>
      <anchor>fd6a5fd350d34f265bd4357fecba50c3</anchor>
      <arglist>(dvdid_hashinfo_t *hi, const uint8_t *buf, size_t size)</arglist>
    </member>
    <member kind="function">
      <type>dvdid_status_t</type>
      <name>dvdid_hashinfo_set_vts01i</name>
      <anchorfile>dvdid2_8h.html</anchorfile>
      <anchor>e867130abf82dc9fc2b2af07caf2870c</anchor>
      <arglist>(dvdid_hashinfo_t *hi, const uint8_t *buf, size_t size)</arglist>
    </member>
    <member kind="function">
      <type>dvdid_status_t</type>
      <name>dvdid_hashinfo_init</name>
      <anchorfile>dvdid2_8h.html</anchorfile>
      <anchor>3340a4c529ed2366238fefcd4e2245b9</anchor>
      <arglist>(dvdid_hashinfo_t *hi)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>dvdid_hashinfo_free</name>
      <anchorfile>dvdid2_8h.html</anchorfile>
      <anchor>98c8f79f89a82c9fc64a6629642df300</anchor>
      <arglist>(dvdid_hashinfo_t *hi)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>export.h</name>
    <path>/home/chris/wrk/.medialibrary-tags/dvdid-0.1.1/include/dvdid/</path>
    <filename>export_8h</filename>
    <member kind="define">
      <type>#define</type>
      <name>DVDID_API</name>
      <anchorfile>export_8h.html</anchorfile>
      <anchor>8cd0fa7859e57f5620adcf5f7c677ef9</anchor>
      <arglist>(type)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DVDID_CALLBACK</name>
      <anchorfile>export_8h.html</anchorfile>
      <anchor>5fcd049a099b2549d434c2c7093c5554</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DVDID_API_VERSION_CURRENT</name>
      <anchorfile>export_8h.html</anchorfile>
      <anchor>6771e8b2cb7711fef5f94aaa9ec0e9b4</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DVDID_API_VERSION_REVISION</name>
      <anchorfile>export_8h.html</anchorfile>
      <anchor>586a962a2aa105dda59547a58dcdb8d8</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DVDID_API_VERSION_AGE</name>
      <anchorfile>export_8h.html</anchorfile>
      <anchor>ae893d59ee9299d82872f29c3695c847</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>dvdid_fileinfo_s</name>
    <filename>structdvdid__fileinfo__s.html</filename>
    <member kind="variable">
      <type>uint64_t</type>
      <name>creation_time</name>
      <anchorfile>structdvdid__fileinfo__s.html</anchorfile>
      <anchor>1ac849e6b3fa2b0081f7233c076bdd1d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>size</name>
      <anchorfile>structdvdid__fileinfo__s.html</anchorfile>
      <anchor>d5f3afbfa0e2e06b68e5fed4bc6b8efd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>name</name>
      <anchorfile>structdvdid__fileinfo__s.html</anchorfile>
      <anchor>6e34b28c198eaa5f62d7937203fe4562</anchor>
      <arglist></arglist>
    </member>
  </compound>
</tagfile>
