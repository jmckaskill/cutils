#include "cutils/zip-writer.h"
#include "cutils/vector.h"
#include "cutils/endian.h"
#include "stream/zip.h"
#include <xz.h> // for crc32
#include <stdbool.h>
#include <assert.h>

struct zip_file {
	struct stream stream;
	char *name;
	uint64_t offset;
	uint64_t uncompressed_len;
	uint64_t compressed_len;
	uint32_t crc;
	uint16_t mtime;
	uint16_t mdate;
	uint16_t namelen;

	stream *source;
	const uint8_t *last_read;
};

struct zip_writer {
	FILE *file;
	struct {
		struct zip_file *v;
		size_t size, cap;
	} entries;
};

static const uint16_t zip_os_unix = 3 << 8;
static const uint16_t zip_version_2_0 = 20;
static const uint16_t zip_version_4_5 = 45;
static const uint16_t zip_flag_use_central_dir = 8;
static const uint16_t zip_flag_utf_8 = 0x800;
static const uint16_t zip_method_deflate = 8;

static uint64_t ftell64(FILE *f) {
#ifdef _MSC_VER
	return _ftelli64(f);
#else
	return ftello(f);
#endif
}

static int fseek64(FILE *f, uint64_t off) {
#ifdef _MSC_VER
	return _fseeki64(f, off, SEEK_SET);
#else
	return fseeko(f, off, SEEK_SET);
#endif
}

struct zip_writer *new_zip_writer(FILE *w) {
	struct zip_writer *z = NEW(struct zip_writer);
	if (!z) {
		return NULL;
	}
	xz_crc32_init();
	z->file = w;
	return z;
}

void free_zip_writer(struct zip_writer *z) {
	if (z) {
		free(z->entries.v);
		free(z);
	}
}

int finish_zip(struct zip_writer *z) {
	// write out the central directory
	// [multiple] file headers
	// zip64 dir end
	// zip64 locator
	// dir end
	uint64_t rec_start = ftell64(z->file);

	for (size_t i = 0; i < z->entries.size; i++) {
		struct zip_file *zf = &z->entries.v[i];
		struct zip_file_header zh;
		write_little_32(zh.sig, ZIP_FILE_HEADER_SIG);
		write_little_16(zh.create_version, zip_os_unix | zip_version_2_0);
		write_little_16(zh.extract_version, zip_version_2_0);
		write_little_16(zh.flags, zip_flag_use_central_dir | zip_flag_utf_8);
		write_little_16(zh.compression_method, zip_method_deflate);
		write_little_16(zh.mtime, zf->mtime);
		write_little_16(zh.mdate, zf->mdate);
		write_little_32(zh.crc32, zf->crc);
		bool zip64 = (zf->compressed_len >= UINT32_MAX || zf->uncompressed_len >= UINT32_MAX || zf->offset >= UINT32_MAX);
		if (zip64) {
			write_little_32(zh.compressed_len, UINT32_MAX);
			write_little_32(zh.uncompressed_len, UINT32_MAX);
			write_little_32(zh.header_off, UINT32_MAX);
		} else {
			write_little_32(zh.compressed_len, (uint32_t)zf->compressed_len);
			write_little_32(zh.uncompressed_len, (uint32_t)zf->uncompressed_len);
			write_little_32(zh.header_off, (uint32_t)zf->offset);
		}
		write_little_16(zh.name_len, zf->namelen);
		write_little_16(zh.extra_len, zip64 ? sizeof(struct zip64_extra) : 0);
		write_little_16(zh.comment_len, 0);
		write_little_16(zh.disk_num, 0);
		write_little_16(zh.internal_attributes, 0);
		write_little_32(zh.external_attributes, 0);

		if (fwrite(&zh, sizeof(zh), 1, z->file) != 1 || fwrite(zf->name, 1, zf->namelen, z->file) != zf->namelen) {
			return -1;
		}

		if (zip64) {
			struct zip64_extra z64;
			write_little_16(z64.h.tag, ZIP64_FILE_EXTRA);
			write_little_16(z64.h.size, sizeof(z64) - sizeof(z64.h));
			write_little_64(z64.compressed_len, zf->compressed_len);
			write_little_64(z64.uncompressed_len, zf->uncompressed_len);
			write_little_64(z64.header_off, zf->offset);
			if (fwrite(&z64, sizeof(z64), 1, z->file) != 1) {
				return -1;
			}
		}
	}

	uint64_t rec_end = ftell64(z->file);
	bool zip64 = (rec_start >= UINT32_MAX || rec_end >= UINT32_MAX || z->entries.size >= UINT16_MAX);

	if (zip64) {
		struct {
			struct zip64_central_dir_end e;
			struct zip64_central_dir_locator l;
		} h;
		write_little_32(h.e.sig, ZIP64_CENTRAL_DIR_END_SIG);
		write_little_64(h.e.end_record_size, sizeof(h.e) - sizeof(h.e.sig) - sizeof(h.e.end_record_size));
		write_little_16(h.e.create_version, zip_version_4_5);
		write_little_16(h.e.extract_version, zip_version_4_5);
		write_little_32(h.e.disk_num, 0);
		write_little_32(h.e.dir_start_disk_num, 0);
		write_little_64(h.e.num_entries_this_disk, z->entries.size);
		write_little_64(h.e.num_entries, z->entries.size);
		write_little_64(h.e.dir_len, rec_end - rec_start);
		write_little_64(h.e.dir_start, rec_start);
		write_little_32(h.l.sig, ZIP64_CENTRAL_DIR_LOCATOR_SIG);
		write_little_32(h.l.dir_start_disk_num, 0);
		write_little_64(h.l.dir_offset, rec_end);
		write_little_32(h.l.total_disks, 1);

		if (fwrite(&h, sizeof(h), 1, z->file) != 1) {
			return -1;
		}
	}

	struct zip_central_dir_end e;
	write_little_32(e.sig, ZIP_CENTRAL_DIR_END_SIG);
	write_little_16(e.disk_num, 0);
	if (zip64) {
		write_little_16(e.num_entries_this_disk, UINT16_MAX);
		write_little_16(e.num_entries, UINT16_MAX);
		write_little_32(e.dir_len, UINT32_MAX);
		write_little_32(e.dir_offset, UINT32_MAX);
		write_little_16(e.comment_len, 0);
	} else {
		write_little_16(e.num_entries_this_disk, (uint16_t)z->entries.size);
		write_little_16(e.num_entries, (uint16_t)z->entries.size);
		write_little_32(e.dir_len, (uint32_t)(rec_end - rec_start));
		write_little_32(e.dir_offset, (uint32_t)rec_start);
		write_little_16(e.comment_len, 0);
	}

	return fwrite(&e, sizeof(e), 1, z->file) != 1;
}

static void close_zip_file(stream *s) {
	(void)s;
}

static const uint8_t *read_zip_file(stream *s, size_t consume, size_t need, size_t *plen) {
	struct zip_file *zf = (struct zip_file*)s;
	zf->uncompressed_len += consume;
	zf->crc = xz_crc32(zf->last_read, consume, zf->crc);
	zf->last_read = zf->source->read(zf->source, consume, need, plen);
	return zf->last_read;
}

int write_zip_file(struct zip_writer *z, stream *source, const char *name, const struct tm *mtime) {
	size_t namelen = strlen(name);
	stream *s = NULL;
	if (namelen > UINT16_MAX) {
		return -1;
	}
	struct zip_file *zf = APPEND_ZERO(&z->entries);
	if (!zf) {
		return -1;
	}
	zf->offset = ftell64(z->file);
	zf->namelen = (uint16_t)namelen;
	zf->name = strdup(name);
	zf->mtime = (uint16_t)(mtime->tm_sec | (mtime->tm_min << 5) | (mtime->tm_hour << 11));
	zf->mdate = (uint16_t)(mtime->tm_mday | ((mtime->tm_mon + 1) << 5) | ((mtime->tm_year - 80) << 9));
	struct zip_local_header h;
	write_little_32(h.sig, ZIP_LOCAL_HEADER);
	write_little_16(h.extract_version, zip_version_2_0);
	write_little_16(h.flags, zip_flag_use_central_dir | zip_flag_utf_8);
	write_little_16(h.compression_method, zip_method_deflate);
	write_little_16(h.mtime, zf->mtime);
	write_little_16(h.mdate, zf->mdate);
	write_little_32(h.crc32, 0);
	write_little_32(h.compressed_len, 0);
	write_little_32(h.uncompressed_len, 0);
	write_little_16(h.name_len, zf->namelen);
	write_little_16(h.extra_len, 0);
	if (fwrite(&h, sizeof(h), 1, z->file) != 1 || fwrite(name, 1, namelen, z->file) != namelen) {
		assert(0);
		goto err;
	}

	// inject our local structure into the stream pipeline so that we can compute
	// the CRC and update the uncompressed_len count
	zf->source = source;
	zf->stream.read = &read_zip_file;
	zf->stream.close = &close_zip_file;

	s = open_deflate(&zf->stream);
	if (!s) {
		assert(0);
		goto err;
	}

	size_t sz = 0;
	for (;;) {
		const uint8_t *p = s->read(s, sz, 1, &sz);
		if (!sz) {
			break;
		}
		if (fwrite(p, 1, sz, z->file) != sz) {
			assert(0);
			goto err;
		}
		zf->compressed_len += sz;
	}

	s->close(s);
	return 0;

err:
	if (s) {
		s->close(s);
	}
	fseek64(z->file, zf->offset);
	z->entries.size--;
	return -1;
}
