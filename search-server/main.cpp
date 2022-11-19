// -------- ������ ��������� ������ ��������� ������� ----------

// ���������� ����������. ����������� �������� ������ ���������� �� ���������� �������, ������� �������� ����� �� ���������.
void TestAddingDocumentToServer()
{
    const int doc_id = 3;
    const string content = "fluffy cat fluffy tail"s;
    const vector<int> ratings = { 1, 2, 3 };
    // ���������� ���
    {
        SearchServer server;
        ASSERT(server.FindTopDocuments("cat"s).empty());
    }
    // �������� ��������
    {
        SearchServer server;
        server.SetStopWords("fluffy"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        int expected_size = 1;
        ASSERT_EQUAL(found_docs.size(), expected_size);
    }
    // ����� ������� � ��������� ��� ���������� ������ ���������
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("dog"s);
        ASSERT(found_docs.empty());
    }
    // ���������� ��������� �� �������� ���������� ������ ���������
    {
        SearchServer server;
        server.AddDocument(doc_id, "   ", DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("   dog"s);
        ASSERT(found_docs.empty());
    }
    // ���������� ������� ��������� ���������� ������ ���������
    {
        SearchServer server;
        server.AddDocument(doc_id, "", DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("dog"s);
        ASSERT(found_docs.empty());
    }
}

// ��������� ����-����. ����-����� ����������� �� ������ ����������.
void TestExcludeStopWordsFromAddedDocumentContent()
{
    const int doc_id = 3;
    const string content = "fluffy cat fluffy tail"s;
    const vector<int> ratings = { 1, 2, 3 };
    // ����� �����, �� ��������� � ���� �����, ������� ������ ��������
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("fluffy"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    // ����� ���� �����, ���������� ������ ���������
    {
        SearchServer server;
        server.SetStopWords("fluffy"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("fluffy"s).empty(), "Stop words must be exclude from document"s);
    }
}

// ��������� �����-����. ���������, ���������� �����-����� ���������� �������, �� ������ ���������� � ���������� ������.
void TestExcludeMinusWordsFromAddedDocumentContent()
{
    const int doc_id = 3;
    const string content = "fluffy cat fluffy tail"s;
    const vector<int> ratings = { 1, 2, 3 };
    // ����� �����, �� ��������� � ����� �����, ������� ������ ��������
    {
        SearchServer server;
        server.SetStopWords("fluffy"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        int expected_size = 1;
        ASSERT_EQUAL(found_docs.size(), expected_size);
    }
    // ����� ����� �����, ���������� ������ ���������
    {
        SearchServer server;
        server.SetStopWords("fluffy"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("fluffy -cat"s).empty());
    }
}

// ������� ����������
void TestMatchDocumentContent()
{
    const int doc_id = 3;
    const string content = "fluffy cat fluffy tail"s;
    const vector<int> ratings = { 1, 2, 3 };
    // ��� �������� ��������� �� ���������� ������� ������ ���� ���������� ��� ����� �� ���������� �������
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        auto [words, document_id] = server.MatchDocument("fluffy cat fluffy tail"s, doc_id);
        int expected_size = 3;
        ASSERT_EQUAL(words.size(), expected_size);
    }
    // ���� ���� ������������ ���� �� �� ������ �����-�����, ������ ������������ ������ ������ ����
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        auto [words, document_id] = server.MatchDocument("fluffy cat fluffy -tail"s, doc_id);
        ASSERT(words.empty());
    }
}

// ���������� ��������� ���������� �� �������������.
void TestRelevanceSortingDocuments()
{
    const int doc_id_1 = 1;
    const string content_1 = "fluffy cat fluffy tail"s;
    const vector<int> ratings_1 = { 1, 2, 3 };

    const int doc_id_2 = 2;
    const string content_2 = "well-groomed dog expressive eyes"s;
    const vector<int> ratings_2 = { 4, 5, 6 };

    const int doc_id_3 = 3;
    const string content_3 = "fluffy well-groomed cat in city"s;
    const vector<int> ratings_3 = { 8, 9, 10 };
    // ������������ ��� ������ ���������� ���������� ������ ���� ������������� � ������� �������� �������������.
    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
        const auto found_docs = server.FindTopDocuments("fluffy well-groomed cat"s);
        ASSERT_EQUAL(found_docs.size(), 3);
        ASSERT_EQUAL(found_docs[0].id, doc_id_1);
        ASSERT_EQUAL(found_docs[1].id, doc_id_3);
        ASSERT_EQUAL(found_docs[2].id, doc_id_2);
    }
}

// ���������� �������� ����������.
void TestRatingDocumentsCalc()
{
    const int doc_id = 3;
    const string content = "fluffy cat fluffy tail"s;
    const vector<int> ratings = { 1, 2, 3 };
    // ������� ������������ ��������� ����� �������� ��������������� ������ ���������.
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("fluffy tail"s);
        double expected_raiting = static_cast<double>((1 + 2 + 3) / 3);
        ASSERT_EQUAL(found_docs[0].rating, expected_raiting);
    }
}

// ���������� ����������� ������ � �������������� ���������, ����������� �������������.
void TestPredicateDocumentSearch()
{
    const int doc_id_1 = 0;
    const string content_1 = "fluffy cat fluffy tail"s;
    const vector<int> ratings_1 = { 1, 2, 3 };

    const int doc_id_2 = 1;
    const string content_2 = "well-groomed dog expressive eyes"s;
    const vector<int> ratings_2 = { 4, 5, 6 };

    const int doc_id_3 = 2;
    const string content_3 = "fluffy well-groomed cat in city"s;
    const vector<int> ratings_3 = { 8, 9, 10 };
    // ��������� � ����������� ������� � ���������� �� id
    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
        const auto found_docs = server.FindTopDocuments("fluffy well-groomed cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        int expected_size = 2;
        ASSERT_EQUAL(found_docs.size(), expected_size);
        ASSERT_EQUAL(found_docs[0].id, doc_id_1);
        ASSERT_EQUAL(found_docs[1].id, doc_id_3);
    }
    // ��������� � ����������� ������� � ���������� �� �������
    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::REMOVED, ratings_2);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
        const auto found_docs = server.FindTopDocuments("fluffy well-groomed cat"s, [](int document_id, DocumentStatus status, int rating) {return status == DocumentStatus::REMOVED; });
        ASSERT_EQUAL(found_docs[0].id, doc_id_2);
    }
}

// ���������� ����������� ������ � �������������� �������, ����������� �������������.
void TestStatusDocumentSearch()
{
    const int doc_id = 3;
    const string content = "fluffy well-groomed cat in city"s;
    const vector<int> ratings = { 1, 2, 3 };
    const DocumentStatus document_status = DocumentStatus::BANNED;
    // ��������� � ����������� ������� �� �������� � �� ���������� ��������
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, document_status, ratings);
        const auto found_docs = server.FindTopDocuments("well-groomed cat in village"s, DocumentStatus::ACTUAL);
        ASSERT(found_docs.empty());
    }
    // ��������� � ����������� ������� � ���������� �������� ��������
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
        const auto found_docs = server.FindTopDocuments("well-groomed cat in village"s, DocumentStatus::BANNED);
        ASSERT(!found_docs.empty());
        ASSERT_EQUAL(found_docs[0].id, doc_id);
    }
}

// ���������� ���������� ������������� ��������� ����������.
void TestRelevanceDocumentCalc()
{
    const int doc_id_1 = 0;
    const vector<int> ratings_1 = { 1 };
    string content_1 = "fluffy well-groomed cat"s;

    const int doc_id_2 = 1;
    const string content_2 = "fluffy well-groomed dog"s;
    const vector<int> ratings_2 = { 2 };
    // ������������� ��������� ���������
    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("well-groomed cat"s);
        double expected_relevance = 0.23104906018664842;
        ASSERT_EQUAL(found_docs[0].relevance, expected_relevance);
    }
}

// ������� TestSearchServer �������� ������ ����� ��� ������� ������
void TestSearchServer()
{
    RUN_TEST(TestAddingDocumentToServer);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusWordsFromAddedDocumentContent);
    RUN_TEST(TestMatchDocumentContent);
    RUN_TEST(TestRelevanceSortingDocuments);
    RUN_TEST(TestRatingDocumentsCalc);
    RUN_TEST(TestPredicateDocumentSearch);
    RUN_TEST(TestStatusDocumentSearch);
    RUN_TEST(TestRelevanceDocumentCalc);
}

// --------- ��������� ��������� ������ ��������� ������� -----------