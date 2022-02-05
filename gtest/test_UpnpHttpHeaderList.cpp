#include "LinkedList.h"
#include "UpnpHttpHeaders.h"
#include "httpparser.h"
#include "list.h"

#include "gtest/gtest.h"
#include <array>
#include <utility>

class HttpHeaderList : public ::testing::Test
{
protected:
        http_message_t msg;
        UpnpListHead header_list;

        HttpHeaderList()
        {
                UpnpListInit(&header_list);
                ListInit(&msg.headers, nullptr, [](void *elem) {
                        auto *hdr = static_cast<http_header_t *>(elem);

                        membuffer_destroy(&hdr->name_buf);
                        membuffer_destroy(&hdr->value);
                        free(hdr);
                });
        }

        ~HttpHeaderList() override
        {
                free_http_headers_list(&header_list);
                ListDestroy(&msg.headers, true);
        }

        void AddHeader(const char name[], const char value[]) noexcept
        {
                auto *hdr = static_cast<http_header_t *>(
                        calloc(1, sizeof(http_header_t)));

                membuffer_assign_str(&hdr->name_buf, name);
                hdr->name = memptr{hdr->name_buf.buf, hdr->name_buf.length};

                membuffer_assign_str(&hdr->value, value);

                ListAddTail(&msg.headers, hdr);
        }

        using HttpHeadersPtr = std::unique_ptr<UpnpHttpHeaders,
                decltype(&UpnpHttpHeaders_delete)>;
        static HttpHeadersPtr MakeUpnpHttpHeaderList()
        {
                return {UpnpHttpHeaders_new(), &UpnpHttpHeaders_delete};
        }
};

TEST_F(HttpHeaderList, Empty)
{
        httpmsg_list_headers(&msg, &header_list);

        const UpnpListIter begin = UpnpListBegin(&header_list);
        ASSERT_EQ(UpnpListNext(&header_list, begin), UpnpListEnd(&header_list));
}

TEST_F(HttpHeaderList, OneElement)
{
        AddHeader("foo", "bar");

        httpmsg_list_headers(&msg, &header_list);

        const UpnpListIter begin = UpnpListBegin(&header_list);

        const auto *header = reinterpret_cast<UpnpHttpHeaders *>(begin);
        ASSERT_STREQ(UpnpHttpHeaders_get_name_cstr(header), "foo");
        ASSERT_STREQ(UpnpHttpHeaders_get_value_cstr(header), "bar");
        ASSERT_EQ(UpnpHttpHeaders_get_resp_cstr(header), nullptr);

        ASSERT_EQ(UpnpListNext(&header_list, begin), UpnpListEnd(&header_list));
}

TEST_F(HttpHeaderList, OneElementMixedCase)
{
        AddHeader("USER-AGENT", "Linux");

        httpmsg_list_headers(&msg, &header_list);

        const UpnpListIter begin = UpnpListBegin(&header_list);

        const auto *header = reinterpret_cast<UpnpHttpHeaders *>(begin);
        ASSERT_STREQ(UpnpHttpHeaders_get_name_cstr(header), "USER-AGENT");
        ASSERT_STREQ(UpnpHttpHeaders_get_value_cstr(header), "Linux");
        ASSERT_EQ(UpnpHttpHeaders_get_resp_cstr(header), nullptr);

        ASSERT_EQ(UpnpListNext(&header_list, begin), UpnpListEnd(&header_list));
}

TEST_F(HttpHeaderList, TwoElements)
{
        AddHeader("USER-AGENT", "Darwin");
        AddHeader("Connection", "close");

        httpmsg_list_headers(&msg, &header_list);

        const UpnpListIter first = UpnpListBegin(&header_list);

        const auto *header = reinterpret_cast<UpnpHttpHeaders *>(first);
        ASSERT_STREQ(UpnpHttpHeaders_get_name_cstr(header), "USER-AGENT");
        ASSERT_STREQ(UpnpHttpHeaders_get_value_cstr(header), "Darwin");
        ASSERT_EQ(UpnpHttpHeaders_get_resp_cstr(header), nullptr);

        const UpnpListIter second = UpnpListNext(&header_list, first);
        ASSERT_NE(second, UpnpListEnd(&header_list));

        header = reinterpret_cast<UpnpHttpHeaders *>(second);
        ASSERT_STREQ(UpnpHttpHeaders_get_name_cstr(header), "Connection");
        ASSERT_STREQ(UpnpHttpHeaders_get_value_cstr(header), "close");
        ASSERT_EQ(UpnpHttpHeaders_get_resp_cstr(header), nullptr);
}

TEST_F(HttpHeaderList, PlentyElements)
{
        /* clang-format off */
        static const std::array<std::pair<const char *, const char *>, 7> elems{{
                {"USER-AGENT", "Darwin"},
                {"Connection", "close"},
                {"content-type", "text"},
                {"pragma", "no-cache"},
                {"pragma", "no-cache"},
                {"CACHE-control", "no-CACHE"},
                {"Accept-Encoding", "gzip"}
        }};
        /* clang-format on */

        for (const auto &elem : elems) {
                AddHeader(elem.first, elem.second);
        }

        httpmsg_list_headers(&msg, &header_list);

        const UpnpListIter begin = UpnpListBegin(&header_list);

        auto idx = 0;
        for (auto it = begin; it != UpnpListEnd(&header_list);
                it = UpnpListNext(begin, it), ++idx) {
                const auto *header = reinterpret_cast<UpnpHttpHeaders *>(it);
                ASSERT_STREQ(UpnpHttpHeaders_get_name_cstr(header),
                        elems[idx].first);
                ASSERT_STREQ(UpnpHttpHeaders_get_value_cstr(header),
                        elems[idx].second);
                ASSERT_EQ(UpnpHttpHeaders_get_resp_cstr(header), nullptr);
        }
        ASSERT_EQ(idx, elems.size());
}

int main(int argc, char **argv)
{
        ::testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
}
