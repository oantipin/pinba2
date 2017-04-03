#ifndef PINBA__REPORT_BY_PACKET_H_
#define PINBA__REPORT_BY_PACKET_H_

#include <boost/noncopyable.hpp>

#include "pinba/globals.h"
#include "pinba/histogram.h"
#include "pinba/packet.h"
#include "pinba/report.h"
#include "pinba/report_util.h"

////////////////////////////////////////////////////////////////////////////////////////////////

struct report_row_data___by_packet_t
{
	uint32_t   req_count;
	uint32_t   timer_count;
	duration_t time_total;
	duration_t ru_utime;
	duration_t ru_stime;
	uint64_t   traffic_kb;
	uint64_t   mem_usage;

	report_row_data___by_packet_t()
	{
		// FIXME: add a failsafe for memset
		memset(this, 0, sizeof(*this));
	}
};

// this is the data we return from report___by_packet_t snapshot
struct report_row___by_packet_t
{
	report_row_data___by_packet_t  data;
	histogram_t                    hv;
};

////////////////////////////////////////////////////////////////////////////////////////////////

struct report_conf___by_packet_t
{
	std::string name;

	duration_t  time_window;      // total time window this report covers (report host uses this for ticking)
	uint32_t    tick_count;         // number of timeslices to store

	uint32_t    hv_bucket_count;  // number of histogram buckets, each bucket is hv_bucket_d 'wide'
	duration_t  hv_bucket_d;      // width of each hv_bucket
};

////////////////////////////////////////////////////////////////////////////////////////////////

struct report___by_packet_t : public report_t
{
	// typedef report_key_t                   key_t;
	typedef report_row_data___by_packet_t  data_t;

	struct item_t
		: private boost::noncopyable
	{
		data_t      data;
		histogram_t hv;

		// XXX: only used by dense_hash_map to default construct the object to be filled
		item_t()
			: data()
			, hv()
		{
		}

		// FIXME: only used by dense_hash_map for set_empty_key()
		//        should not be called often with huge histograms, so expect it to be ok :(
		//        sparsehash with c++11 support (https://github.com/sparsehash/sparsehash-c11) fixes this
		//        but gcc 4.9.4 doesn't support the type_traits it requires
		//        so live this is for now, but probably - move to gcc6 or something
		item_t(item_t const& other)
			: data(other.data)
			, hv(other.hv)
		{
		}

		item_t(item_t&& other)
		{
			*this = std::move(other); // operator=()
		}

		void operator=(item_t&& other)
		{
			data = other.data;          // a copy
			hv   = std::move(other.hv); // real move
		}

		void data_increment(packet_t *packet)
		{
			data.req_count   += 1;
			data.timer_count += packet->timer_count;
			data.time_total  += packet->request_time;
			data.ru_utime    += packet->ru_utime;
			data.ru_stime    += packet->ru_stime;
			data.traffic_kb  += packet->doc_size;
			data.mem_usage   += packet->memory_peak;
		}

		void hv_increment(packet_t *packet, uint32_t hv_bucket_count, duration_t hv_bucket_d)
		{
			hv.increment({hv_bucket_count, hv_bucket_d}, packet->request_time);
		}

		void merge_other(item_t const& other)
		{
			// data
			data.req_count   += other.data.req_count;
			data.timer_count += other.data.timer_count;
			data.time_total  += other.data.time_total;
			data.ru_utime    += other.data.ru_utime;
			data.ru_stime    += other.data.ru_stime;
			data.traffic_kb  += other.data.traffic_kb;
			data.mem_usage   += other.data.mem_usage;

			// hv
			hv.merge_other(other.hv);
		}
	};

public: // ticks

	using ticks_t       = ticks_ringbuffer_t<item_t>;
	using tick_t        = ticks_t::tick_t;
	using ticks_list_t  = ticks_t::ringbuffer_t;

public: // snapshot

	struct snapshot_traits
	{
		using src_ticks_t = ticks_list_t;
		using hashtable_t = std::array<report_row___by_packet_t, 1>; // array to get iterators 'for free'

		static report_key_t key_at_position(hashtable_t const&, hashtable_t::iterator const& it)    { return {}; }
		static void*        value_at_position(hashtable_t const&, hashtable_t::iterator const& it)  { return (void*)it; }
		static histogram_t* hv_at_position(hashtable_t const&, hashtable_t::iterator const& it)     { return &it->hv; }

		// merge from src ringbuffer to snapshot data
		static void merge_ticks_into_data(pinba_globals_t*, report_info_t& rinfo, src_ticks_t const& ticks, hashtable_t& to)
		{
			for (auto const& tick : ticks)
			{
				if (!tick)
					continue;

				auto const& src = tick->data;
				auto      & dst = to[0];

				dst.data.req_count   += src.data.req_count;
				dst.data.timer_count += src.data.timer_count;
				dst.data.time_total  += src.data.time_total;
				dst.data.ru_utime    += src.data.ru_utime;
				dst.data.ru_stime    += src.data.ru_stime;
				dst.data.traffic_kb  += src.data.traffic_kb;
				dst.data.mem_usage   += src.data.mem_usage;

				if (rinfo.hv_enabled)
				{
					dst.hv.merge_other(src.hv);
				}
			}
		}
	};

	using snapshot_t = report_snapshot__impl_t<snapshot_traits>;

public:

	report___by_packet_t(pinba_globals_t *globals, report_conf___by_packet_t const& conf)
		: globals_(globals)
		, conf_(conf)
		, ticks_(conf.tick_count)
	{
		info_ = report_info_t {
			.name            = conf_.name,
			.kind            = REPORT_KIND__BY_PACKET_DATA,
			.time_window     = conf_.time_window,
			.tick_count      = conf_.tick_count,
			.n_key_parts     = 0,
			.hv_enabled      = (conf_.hv_bucket_count > 0),
			.hv_kind         = HISTOGRAM_KIND__HASHTABLE,
			.hv_bucket_count = conf_.hv_bucket_count,
			.hv_bucket_d     = conf_.hv_bucket_d,
		};
	}

	virtual str_ref name() const override
	{
		return info_.name;
	}

	virtual report_info_t const* info() const override
	{
		return &info_;
	}

	virtual int kind() const override
	{
		return info_.kind;
	}

public:

	virtual void ticks_init(timeval_t curr_tv) override
	{
		ticks_.init(curr_tv);
	}

	virtual void tick_now(timeval_t curr_tv) override
	{
		ticks_.tick(curr_tv);
	}

	virtual report_snapshot_ptr get_snapshot() override
	{
		return meow::make_unique<snapshot_t>(globals_, ticks_.get_internal_buffer(), info_);
	}

public:

	virtual void add(packet_t *packet) override
	{
		item_t& item = ticks_.current().data;
		item.data_increment(packet);

		if (info_.hv_enabled)
		{
			item.hv_increment(packet, conf_.hv_bucket_count, conf_.hv_bucket_d);
		}
	}

	virtual void add_multi(packet_t **packets, uint32_t packet_count) override
	{
		for (uint32_t i = 0; i < packet_count; ++i)
			this->add(packets[i]);
	}

// private:
protected:
	pinba_globals_t             *globals_;
	report_conf___by_packet_t   conf_;

	report_info_t               info_;

	ticks_t                     ticks_;
};

////////////////////////////////////////////////////////////////////////////////////////////////

#endif // PINBA__REPORT_BY_PACKET_H_