/* gpgsplit.c - An OpenPGP signature packet splitting tool
 * Copyright (C) 2001, 2002, 2003, 2006 Free Software Foundation, Inc.
 *
 * This file is part of of CVS
 * (originally derived from gpgsplit.c distributed with GnuPG, but much
 * has been rewritten from scratch using RFC 2440).
 *
 * GnuPG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GnuPG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

/* Verify interface.  */
#include "gpg.h"

/* ANSI C Headers.  */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* GNULIB Headers.  */
#include "error.h"
#include "xalloc.h"

/* CVS headers.  */
#include "parseinfo.h"



typedef enum {
	PKT_NONE	   =0,
	PKT_PUBKEY_ENC	   =1, /* public key encrypted packet */
	PKT_SIGNATURE	   =2, /* secret key encrypted packet */
	PKT_SYMKEY_ENC	   =3, /* session key packet (OpenPGP)*/
	PKT_ONEPASS_SIG    =4, /* one pass sig packet (OpenPGP)*/
	PKT_SECRET_KEY	   =5, /* secret key */
	PKT_PUBLIC_KEY	   =6, /* public key */
	PKT_SECRET_SUBKEY  =7, /* secret subkey (OpenPGP) */
	PKT_COMPRESSED	   =8, /* compressed data packet */
	PKT_ENCRYPTED	   =9, /* conventional encrypted data */
	PKT_MARKER	  =10, /* marker packet (OpenPGP) */
	PKT_PLAINTEXT	  =11, /* plaintext data with filename and mode */
	PKT_RING_TRUST	  =12, /* keyring trust packet */
	PKT_USER_ID	  =13, /* user id packet */
	PKT_PUBLIC_SUBKEY =14, /* public subkey (OpenPGP) */
	PKT_OLD_COMMENT   =16, /* comment packet from an OpenPGP draft */
	PKT_ATTRIBUTE     =17, /* PGP's attribute packet */
	PKT_ENCRYPTED_MDC =18, /* integrity protected encrypted data */
	PKT_MDC 	  =19, /* manipulation detection code packet */
	PKT_COMMENT	  =61, /* new comment packet (private) */
        PKT_GPG_CONTROL   =63  /* internal control packet */
} pkttype_t;



static const char *
pkttype_to_string (int pkttype)
{
  const char *s;

  switch (pkttype)
    {
    case PKT_PUBKEY_ENC    : s = "pk_enc"; break;
    case PKT_SIGNATURE     : s = "sig"; break;
    case PKT_SYMKEY_ENC    : s = "sym_enc"; break;
    case PKT_ONEPASS_SIG   : s = "onepass_sig"; break;
    case PKT_SECRET_KEY    : s = "secret_key"; break;
    case PKT_PUBLIC_KEY    : s = "public_key"; break;
    case PKT_SECRET_SUBKEY : s = "secret_subkey"; break;
    case PKT_COMPRESSED    : s = "compressed";
      break;
    case PKT_ENCRYPTED     : s = "encrypted"; break;
    case PKT_MARKER	   : s = "marker"; break;
    case PKT_PLAINTEXT     : s = "plaintext"; break;
    case PKT_RING_TRUST    : s = "ring_trust"; break;
    case PKT_USER_ID       : s = "user_id"; break;
    case PKT_PUBLIC_SUBKEY : s = "public_subkey"; break;
    case PKT_OLD_COMMENT   : s = "old_comment"; break;
    case PKT_ATTRIBUTE     : s = "attribute"; break;
    case PKT_ENCRYPTED_MDC : s = "encrypted_mdc"; break;
    case PKT_MDC 	   : s = "mdc"; break;
    case PKT_COMMENT       : s = "comment"; break;
    case PKT_GPG_CONTROL   : s = "gpg_control"; break;
    default                : s = "unknown"; break;
    }
  return s;
}



static inline int
read_u8 (struct buffer *bpin, uint8_t *rn)
{
  char *tmp;
  size_t got;
  int rc;

  if ((rc = buf_read_data (bpin, 1, &tmp, &got)) < 0)
    return rc;
  assert (got == 1);
  *rn = *tmp;
  return 0;
}



static inline int
read_u16 (struct buffer *bpin, uint16_t *rn)
{
  uint8_t tmp;
  int rc;

  if ((rc = read_u8 (bpin, &tmp)))
    return rc;
  *rn = tmp << 8;
  if ((rc = read_u8 (bpin, &tmp)))
    return rc;
  *rn |= tmp;
  return 0;
}



static inline int
read_u32 (struct buffer *bpin, uint32_t *rn)
{
  uint16_t tmp;
  int rc;

  if ((rc = read_u16 (bpin, &tmp)))
    return rc;
  *rn = tmp << 16;
  if ((rc = read_u16 (bpin, &tmp)))
    return rc;
  *rn |= tmp;
  return 0;
}



static inline int
read_u64 (struct buffer *bpin, uint64_t *rn)
{
  uint32_t tmp;
  int rc;

  if ((rc = read_u32 (bpin, &tmp)))
    return rc;
  /* This next is done in two steps to suppress a GCC warning.  */
  *rn = tmp;
  *rn <<= 32;
  if ((rc = read_u32 (bpin, &tmp)))
    return rc;
  *rn |= tmp;
  return 0;
}



/* hdr must point to a buffer large enough to hold all header bytes */
static int
write_part (struct buffer *bpin, struct buffer *bpout, unsigned long pktlen,
            int pkttype, int partial, uint8_t *hdr, size_t hdrlen)
{
  char *tmp;
  int rc;

  while (partial)
    {
      uint16_t partlen;
      
      assert (partial == 2);
      /* old gnupg */
      assert (!pktlen);
      if ((rc = read_u16 (bpin, &partlen)))
	return rc;
      hdr[hdrlen++] = partlen >> 8;
      hdr[hdrlen++] = partlen;
      buf_output (bpout, (char *)hdr, hdrlen);
      hdrlen = 0;
      if (!partlen)
	partial = 0; /* end of packet */
      while (partlen)
	{
	  size_t got;
	  if ((rc = buf_read_data (bpin, partlen, &tmp, &got)) < 0)
	    return rc;
	  assert (got);  /* Blocking buffers cannot return 0 bytes.  */
	  buf_output (bpout, tmp, got);
	  partlen -= got;
	}
    }

  if (hdrlen)
    buf_output (bpout, (char *)hdr, hdrlen);
  
  /* standard packet or last segment of partial length encoded packet */
  while (pktlen)
    {
      size_t got;
      if ((rc = buf_read_data (bpin, pktlen, &tmp, &got)) < 0)
	return rc;
      assert (got);  /* Blocking buffers cannot return 0 bytes.  */
      buf_output (bpout, tmp, got);
      pktlen -= got;
    }

  return 0;
}



/* Read a single signature packet header from BPIN.
 *
 * INPUTS
 *   BPIN	Pointer to the buffer to read from.
 *
 * OUTPUTS
 *   *PKTTYPE	PGP Packet type read from buffer.
 *   *PKTLEN	Length of data remaining in buffer (PKTLEN + HEADER_LEN =
 *   		Full length of OpenPGP packet).
 *   PARTIAL	1 if this is a partial packet, 0 otherwise.
 *   HEADER	Copy of header block from PGP packet.
 *   HEADER_LEN	Length of HEADER.
 *
 * RETURNS
 *   0		On success.
 *   -1		If EOF is encountered before a full packet is read.
 *   -2		On memory allocation errors from buf_read_data().
 */
int
parse_header (struct buffer *bpin, int *pkttype, uint32_t *pktlen,
    	      int *partial, uint8_t *header, int *header_len)
{
  int ctb;
  int header_idx = 0;
  int lenbytes;
  int rc;
  uint8_t c;

  *pktlen = 0;
  *partial = 0;

  if ((rc = read_u8 (bpin, &c)))
    return rc;

  ctb = c;

  header[header_idx++] = ctb;
  
  if (!(ctb & 0x80))
    {
      error (0, 0, "invalid CTB %02x\n", ctb );
      return 1;
    }
  if ( (ctb & 0x40) )
    {
      /* new CTB */
      *pkttype =  (ctb & 0x3f);

      if ((rc = read_u8 (bpin, &c)))
	return rc;

      header[header_idx++] = c;

      if ( c < 192 )
        *pktlen = c;
      else if ( c < 224 )
        {
          *pktlen = (c - 192) * 256;
	  if ((rc = read_u8 (bpin, &c)))
	    return rc;
          header[header_idx++] = c;
          *pktlen += c + 192;
	}
      else if ( c == 255 ) 
        {
	  if ((rc = read_u32 (bpin, pktlen)))
	    return rc;
          header[header_idx++] = *pktlen >> 24;
          header[header_idx++] = *pktlen >> 16;
          header[header_idx++] = *pktlen >> 8;
          header[header_idx++] = *pktlen; 
	}
      else
        { /* partial body length */
          *pktlen = c;
          *partial = 1;
	}
    }
  else
    {
      *pkttype = (ctb>>2)&0xf;

      lenbytes = ((ctb&3)==3)? 0 : (1<<(ctb & 3));
      if (!lenbytes )
	{
	  *pktlen = 0; /* don't know the value */
	  *partial = 2; /* the old GnuPG partial length encoding */
	}
      else
	{
	  for (; lenbytes; lenbytes--) 
	    {
	      *pktlen <<= 8;
	      if ((rc = read_u8 (bpin, &c)))
		return rc;
	      header[header_idx++] = c;
	      
	      *pktlen |= c;
	    }
	}
    }

  *header_len = header_idx;
  return 0;
}



/* Read a single signature packet from BPIN, copying it to BPOUT.
 *
 * RETURNS
 *   0		On success.
 *   -1		If EOF is encountered before a full packet is read.
 *   -2		On memory allocation errors from buf_read_data().
 *
 * ERRORS
 *   Aside from the error returns above, buf_output() can call its memory
 *   failure function on memory allocation failures, which could exit.
 */
int
next_signature (struct buffer *bpin, struct buffer *bpout)
{
  int pkttype;
  uint32_t pktlen;
  int partial;
  uint8_t header[20];
  int header_len = sizeof header;
  int rc;

  if ((rc = parse_header (bpin, &pkttype, &pktlen, &partial, header,
			  &header_len)))
    return rc;

  if (pkttype != PKT_SIGNATURE)
    error (1, 0, "Inavlid OpenPGP packet type (%s)",
	   pkttype_to_string (pkttype));

  return write_part (bpin, bpout, pktlen, pkttype, partial,
                     header, header_len);
}



struct openpgp_signature_subpacket
{
  uint8_t *raw;
  size_t rawlen;
  uint8_t type;
  union
  {
    time_t ctime;
    uint64_t keyid;
  } u;
};

/* Parse a single signature version 4 subpacket from BPIN, populating
 * structure at SPOUT.
 *
 * RETURNS
 *   0		On success.
 *   -1		If EOF is encountered before a full packet is read.
 *   -2		On memory allocation errors from buf_read_data().
 *
 * ERRORS
 *   Aside from the error returns above, buf_output() can call its memory
 *   failure function on memory allocation failures, which could exit.
 */
static int
parse_signature_subpacket (struct buffer *bpin,
			   struct openpgp_signature_subpacket *spout)
{
  int rc;
  uint8_t c;
  uint32_t tmp32;
  uint32_t splen;
  size_t raw_idx = 0;

  /* Enough to store the subpacket header.  */
  spout->raw = xmalloc (5);

  if ((rc = read_u8 (bpin, &c)))
    return rc;

  spout->raw[raw_idx++] = c;

  if (c < 192)
    splen = c;
  else if (c < 255)
    {
      splen = (c - 192) << 8;
      if ((rc = read_u8 (bpin, &c)))
	return rc;
      spout->raw[raw_idx++] = c;
      splen += c + 192;
    }
  else /* c == 255 */
    {
      if ((rc = read_u32 (bpin, &splen)))
	return rc;
      spout->raw[raw_idx++] = splen >> 24;
      spout->raw[raw_idx++] = splen >> 16;
      spout->raw[raw_idx++] = splen >> 8;
      spout->raw[raw_idx++] = splen; 
    }

  if (splen == 0)
    error (1, 0, "Received zero length subpacket in OpenPGP signature.");

  /* Allocate enough bytes for the rest of the subpacket.  */
  spout->raw = xrealloc (spout->raw, raw_idx + splen);

  /* Read the subpacket type.  */
  if ((rc = read_u8 (bpin, &c)))
    return rc;
  spout->raw[raw_idx++] = c;
  splen -= 1;
  spout->type = c;

  switch (c)
  {
    /* Signature creation time.  */
    case 2:
      if (splen != 4)
	error (1, 0, "Missized signature subpacket (%lu)", (long)splen);

      if ((rc = read_u32 (bpin, &tmp32)))
	return rc;
      spout->u.ctime = tmp32;
      spout->raw[raw_idx++] = (tmp32 >> 24) & 0xFF;
      spout->raw[raw_idx++] = (tmp32 >> 16) & 0xFF;
      spout->raw[raw_idx++] = (tmp32 >> 8) & 0xFF;
      spout->raw[raw_idx++] = tmp32 & 0xFF;
      break;

    /* Signer's key ID.  */
    case 16:
      if (splen != 8)
	error (1, 0, "Missized signature subpacket (%lu)", (long)splen);

      if ((rc = read_u64 (bpin, &spout->u.keyid)))
	return rc;
      spout->raw[raw_idx++] = (spout->u.keyid >> 56) & 0xFF;
      spout->raw[raw_idx++] = (spout->u.keyid >> 48) & 0xFF;
      spout->raw[raw_idx++] = (spout->u.keyid >> 40) & 0xFF;
      spout->raw[raw_idx++] = (spout->u.keyid >> 32) & 0xFF;
      spout->raw[raw_idx++] = (spout->u.keyid >> 24) & 0xFF;
      spout->raw[raw_idx++] = (spout->u.keyid >> 16) & 0xFF;
      spout->raw[raw_idx++] = (spout->u.keyid >> 8) & 0xFF;
      spout->raw[raw_idx++] = spout->u.keyid & 0xFF;
      break;

    /* Store but ignore other subpacket types.  */
    default:
      while (splen)
	{
	  size_t got;
	  char *tmp;
	  if ((rc = buf_read_data (bpin, splen, &tmp, &got)) < 0)
	    return rc;
	  assert (got);  /* Blocking buffers cannot return 0 bytes.  */
	  memcpy (spout->raw + raw_idx, tmp, got);
	  raw_idx += got;
	  splen -= got;
	}
      break;
  }

  /* This should be the original length read plus the 1, 2, or 5 bytes in the
   * length header itself.
   */
  spout->rawlen = raw_idx;
  return 0;
}



/* Parse a single signature packet from BPIN, populating structure at SPOUT.
 *
 * RETURNS
 *   0		On success.
 *   -1		If EOF is encountered before a full packet is read.
 *   -2		On memory allocation errors from buf_read_data().
 *
 * ERRORS
 *   Aside from the error returns above, buf_output() can call its memory
 *   failure function on memory allocation failures, which could exit.
 */
int
parse_signature (struct buffer *bpin, struct openpgp_signature *spout)
{
  int pkttype;
  uint32_t pktlen;
  int partial;
  uint8_t header[20];
  int header_len = sizeof header;
  int rc;
  uint8_t c;
  uint16_t tsplen;
  uint32_t tmp32;
  size_t raw_idx = 0;
  bool got_ctime;
  bool got_keyid;

  if ((rc = parse_header (bpin, &pkttype, &pktlen, &partial, header,
			  &header_len)))
    return rc;

  if (partial)
    error (1, 0, "Unhandled OpenPGP packet type (partial)");
  if (pkttype != PKT_SIGNATURE)
    error (1, 0, "Unhandled OpenPGP packet type (%s)",
	   pkttype_to_string (pkttype));

  spout->raw = xmalloc (header_len + pktlen);
  memcpy (spout->raw + raw_idx, header, header_len);
  raw_idx += header_len;

  if (pktlen < 19)
    error (1, 0, "Malformed OpenPGP signature packet (too short)");

  if ((rc = read_u8 (bpin, &c)))
    return rc;
  spout->raw[raw_idx++] = c;
  pktlen -= 1;

  if (c != 3 && c != 4)
    error (1, 0, "Unhandled OpenPGP signature version (%hhu)", c);

  switch (c)
  {
    case 3:
      /* RFC 2440 Section 5.2.2 Version 4 Signature Packet Format  */
      if ((rc = read_u8 (bpin, &c)))
	return rc;
      spout->raw[raw_idx++] = c;
      pktlen -= 1;

      if (c != 5)
	error (1, 0, "Malformed OpenPGP signature (type/time length invalid)");

      if ((rc = read_u8 (bpin, &c)))
	return rc;
      spout->raw[raw_idx++] = c;
      pktlen -= 1;

      if (c & 0xF0)
	error (1, 0, "Unhandled OpenPGP signature type (%hhu)", c);

      /* Read the creation time.  */
      if ((rc = read_u32 (bpin, &tmp32)))
	return rc;
      spout->ctime = tmp32;
      spout->raw[raw_idx++] = (tmp32 >> 24) & 0xFF;
      spout->raw[raw_idx++] = (tmp32 >> 16) & 0xFF;
      spout->raw[raw_idx++] = (tmp32 >> 8) & 0xFF;
      spout->raw[raw_idx++] = tmp32 & 0xFF;
      pktlen -= 4;

      /* Read the key ID.  */
      if ((rc = read_u64 (bpin, &spout->keyid)))
	return rc;
      spout->raw[raw_idx++] = (spout->keyid >> 56) & 0xFF;
      spout->raw[raw_idx++] = (spout->keyid >> 48) & 0xFF;
      spout->raw[raw_idx++] = (spout->keyid >> 40) & 0xFF;
      spout->raw[raw_idx++] = (spout->keyid >> 32) & 0xFF;
      spout->raw[raw_idx++] = (spout->keyid >> 24) & 0xFF;
      spout->raw[raw_idx++] = (spout->keyid >> 16) & 0xFF;
      spout->raw[raw_idx++] = (spout->keyid >> 8) & 0xFF;
      spout->raw[raw_idx++] = spout->keyid & 0xFF;
      pktlen -= 8;
      break;

    case 4:
      /* RFC 2440 Section 5.2.3 Version 4 Signature Packet Format  */
      got_ctime = false;
      got_keyid = false;

      /* Discard one-octet signature type.  */
      if ((rc = read_u8 (bpin, &c)))
	return rc;
      spout->raw[raw_idx++] = c;
      pktlen -= 1;

      /* Discard one-octet public key algorithm.  */
      if ((rc = read_u8 (bpin, &c)))
	return rc;
      spout->raw[raw_idx++] = c;
      pktlen -= 1;

      /* Discard one-octet hash algorithm.  */
      if ((rc = read_u8 (bpin, &c)))
	return rc;
      spout->raw[raw_idx++] = c;
      pktlen -= 1;

      /* Read the hashed subpacket length.  */
      if ((rc = read_u16 (bpin, &tsplen)))
	return rc;
      spout->raw[raw_idx++] = (tsplen >> 8) & 0xFF;
      spout->raw[raw_idx++] = tsplen & 0xFF;
      pktlen -= 2;

      /* Process hashed subpackets.  */
      while (tsplen > 0)
      {
	struct openpgp_signature_subpacket sp;

	if (got_ctime && got_keyid)
	{
	  while (tsplen)
	    {
	      size_t got;
	      char *tmp;
	      if ((rc = buf_read_data (bpin, tsplen, &tmp, &got)) < 0)
		return rc;
	      assert (got);  /* Blocking buffers cannot return 0 bytes.  */
	      memcpy (spout->raw + raw_idx, tmp, got);
	      raw_idx += got;
	      tsplen -= got;
	      pktlen -= got;
	    }
	  break;
	}

	if ((rc = parse_signature_subpacket (bpin, &sp)))
	  return rc;

	if (sp.rawlen > tsplen)
	  error (1, 0,
"Signature subpacket length exceeds total hashed subpackets length.");

	memcpy (spout->raw + raw_idx, sp.raw, sp.rawlen);
	free (sp.raw);
	raw_idx += sp.rawlen;
	pktlen -= sp.rawlen;
	tsplen -= sp.rawlen;

	switch (sp.type)
	{
	  case 2:
	    spout->ctime = sp.u.ctime;
	    got_ctime = true;
	    break;
	  case 16:
	    spout->keyid = sp.u.keyid;
	    got_keyid = true;
	    break;
	  default:
	    break;
	}
      }

      /* Read the unhashed subpacket length.  */
      if ((rc = read_u16 (bpin, &tsplen)))
	return rc;
      spout->raw[raw_idx++] = (tsplen >> 8) & 0xFF;
      spout->raw[raw_idx++] = tsplen & 0xFF;
      pktlen -= 2;

      /* Process unhashed subpackets.  */
      while (tsplen > 0)
      {
	struct openpgp_signature_subpacket sp;

	if (got_ctime && got_keyid)
	{
	  while (tsplen)
	    {
	      size_t got;
	      char *tmp;
	      if ((rc = buf_read_data (bpin, tsplen, &tmp, &got)) < 0)
		return rc;
	      assert (got);  /* Blocking buffers cannot return 0 bytes.  */
	      memcpy (spout->raw + raw_idx, tmp, got);
	      raw_idx += got;
	      tsplen -= got;
	      pktlen -= got;
	    }
	  break;
	}

	if ((rc = parse_signature_subpacket (bpin, &sp)))
	  return rc;

	if (sp.rawlen > tsplen)
	  error (1, 0,
"Signature subpacket length exceeds total unhashed subpackets length.");

	memcpy (spout->raw + raw_idx, sp.raw, sp.rawlen);
	free (sp.raw);
	raw_idx += sp.rawlen;
	pktlen -= sp.rawlen;
	tsplen -= sp.rawlen;

	switch (sp.type)
	{
	  case 2:
	    error (1, 0, "Creation time found in unhashed subpacket.");
	    /* Exits here, but who knows if that confuses the optimizer.  */
	    break;

	  case 16:
	    spout->keyid = sp.u.keyid;
	    got_keyid = true;
	    break;

	  default:
	    break;
	}
      }

      if (!got_ctime)
	error (1, 0, "Signature did not contain creation time.");
      if (!got_keyid)
	error (1, 0, "Signature did not contain keyid.");
      break;
  }

  /* Don't need the rest of the packet yet.  */
  while (pktlen)
    {
      size_t got;
      char *tmp;
      if ((rc = buf_read_data (bpin, pktlen, &tmp, &got)) < 0)
	return rc;
      assert (got);  /* Blocking buffers cannot return 0 bytes.  */
      memcpy (spout->raw + raw_idx, tmp, got);
      raw_idx += got;
      pktlen -= got;
    }

  spout->rawlen = raw_idx;
  return 0;
}



static char *openpgp_textmode;

void
set_openpgp_textmode (const char *textmode)
{
    assert (textmode);
    if (openpgp_textmode) free (openpgp_textmode);
    openpgp_textmode = xstrdup (textmode);
}



/* Return OPENPGP_TEXTMODE from the command line if it exists, else if
 * CONFIG->OPENPGPTEXTMODE is set, return that, else if OPENPGP_TEXTMODE
 * is set in the CURRENT_PARSED_ROOT return that, else return the default
 * mode.
 *
 * If the value to be returned is the empty string, then return NULL so that
 * format_cmdline will skip the argument rather than passing an empty string.
 */
const char *
get_openpgp_textmode (void)
{
    const char *tmp = NULL;

    if (openpgp_textmode)
	tmp = openpgp_textmode;
    else if (config && config->OpenPGPTextmode)
	tmp = config->OpenPGPTextmode;
    else if (current_parsed_root->openpgp_textmode)
	tmp = current_parsed_root->openpgp_textmode;
    else
	tmp = DEFAULT_SIGN_TEXTMODE;

    if (tmp && !strlen (tmp)) return NULL;
    /* else */ return tmp;
}



#ifndef HAVE_LONG_LONG
char *
gpg_keyid2longstring (uint64_t keyid)
{
    if (!(keyid >> 32))
	return Xasprintf ("0x%lx", (unsigned long)keyid);
    /* else */
    return Xasprintf ("0x%lx%lx",
		      (unsigned long)(keyid >> 32),
		      (unsigned long)(keyid & 0xFFFFFFFF));
}
#endif /* !HAVE_LONG_LONG */
