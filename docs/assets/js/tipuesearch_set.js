
/*
Tipue Search 6.0
Copyright (c) 2017 Tipue
Tipue Search is released under the MIT License
http://www.tipue.com/search
*/


/*
Stop words
Stop words list from http://www.ranks.nl/stopwords
*/

var tipuesearch_stop_words = ["a", "about", "above", "after", "again", "against", "all", "am", "an", "and", "any", "are", "aren't", "as", "at", "be", "because", "been", "before", "being", "below", "between", "both", "but", "by", "can't", "cannot", "could", "couldn't", "did", "didn't", "do", "does", "doesn't", "doing", "don't", "down", "during", "each", "few", "for", "from", "further", "had", "hadn't", "has", "hasn't", "have", "haven't", "having", "he", "he'd", "he'll", "he's", "her", "here", "here's", "hers", "herself", "him", "himself", "his", "how", "how's", "i", "i'd", "i'll", "i'm", "i've", "if", "in", "into", "is", "isn't", "it", "it's", "its", "itself", "let's", "me", "more", "most", "mustn't", "my", "myself", "no", "nor", "not", "of", "off", "on", "once", "only", "or", "other", "ought", "our", "ours", "ourselves", "out", "over", "own", "same", "shan't", "she", "she'd", "she'll", "she's", "should", "shouldn't", "so", "some", "such", "than", "that", "that's", "the", "their", "theirs", "them", "themselves", "then", "there", "there's", "these", "they", "they'd", "they'll", "they're", "they've", "this", "those", "through", "to", "too", "under", "until", "up", "very", "was", "wasn't", "we", "we'd", "we'll", "we're", "we've", "were", "weren't", "what", "what's", "when", "when's", "where", "where's", "which", "while", "who", "who's", "whom", "why", "why's", "with", "won't", "would", "wouldn't", "you", "you'd", "you'll", "you're", "you've", "your", "yours", "yourself", "yourselves", "これ", "それ", "あれ", "この", "その", "あの", "ここ", "そこ", "あそこ", "こちら", "どこ", "だれ", "なに", "なん", "何", "私", "貴方", "貴方方", "我々", "私達", "あの人", "あのかた", "彼女", "彼", "です", "あります", "おります", "います", "は", "が", "の", "に", "を", "で", "え", "から", "まで", "より", "も", "どの", "と", "し", "それで", "しかし" ];


// Word replace

var tipuesearch_replace = {'words': [
     {'word': 'tip', 'replace_with': 'tipue'},
     {'word': 'javscript', 'replace_with': 'javascript'},
     {'word': 'jqeury', 'replace_with': 'jquery'}
]};


// Weighting

var tipuesearch_weight = {'weight': [
     {'url': 'http://www.tipue.com', 'score': 20},
     {'url': 'http://www.tipue.com/search', 'score': 30},
     {'url': 'http://www.tipue.com/is', 'score': 10}
]};


// Illogical stemming

var tipuesearch_stem = {'words': [
     {'word': 'e-mail', 'stem': 'email'},
     {'word': 'javascript', 'stem': 'jquery'},
     {'word': 'javascript', 'stem': 'js'}
]};


// Related searches

var tipuesearch_related = {'searches': [
     {'search': 'tipue', 'related': 'Tipue Search'},
     {'search': 'tipue', 'before': 'Tipue Search', 'related': 'Getting Started'},
     {'search': 'tipue', 'before': 'Tipue', 'related': 'jQuery'},
     {'search': 'tipue', 'before': 'Tipue', 'related': 'Blog'}
]};


// Internal strings

if (language == "ja") {
    var tipuesearch_string_1 = 'タイトルなし';
    var tipuesearch_string_2 = '検索結果を表示: ';
    var tipuesearch_string_3 = '代わりに検索: ';
    var tipuesearch_string_4 = '1 件';
    var tipuesearch_string_5 = '件';
    var tipuesearch_string_6 = '戻る';
    var tipuesearch_string_7 = '＞＞';
    var tipuesearch_string_8 = '該当するページが見つかりません。';
    var tipuesearch_string_9 = '一般的な言葉はほとんど無視されます。';
    var tipuesearch_string_10 = '入力が短すぎます。';
    var tipuesearch_string_11 = '1文字以上である必要があります。';
    var tipuesearch_string_12 = 'である必要があります';
    var tipuesearch_string_13 = '文字以上である必要があります。';
    var tipuesearch_string_14 = '秒';
    var tipuesearch_string_15 = '関連する検索: ';
} else if(language == "ch") {
    var tipuesearch_string_1 = '没有标题';
    var tipuesearch_string_2 = '显示搜索';
    var tipuesearch_string_3 = '替代搜索';
    var tipuesearch_string_4 = '1 条结果';
    var tipuesearch_string_5 = '搜索结果';
    var tipuesearch_string_6 = '返回';
    var tipuesearch_string_7 = '更多';
    var tipuesearch_string_8 = '没有结果';
    var tipuesearch_string_9 = '太长的词组被忽略';
    var tipuesearch_string_10 = '查询内容太短';
    var tipuesearch_string_11 = '应该多于一个字.';
    var tipuesearch_string_12 = '应该';
    var tipuesearch_string_13 = '个词语或更多';
    var tipuesearch_string_14 = '秒';
    var tipuesearch_string_15 = '关于搜索';
}else {
    var tipuesearch_string_1 = 'No title';
    var tipuesearch_string_2 = 'Showing results for';
    var tipuesearch_string_3 = 'Search instead for';
    var tipuesearch_string_4 = '1 result';
    var tipuesearch_string_5 = 'results';
    var tipuesearch_string_6 = 'Back';
    var tipuesearch_string_7 = 'More';
    var tipuesearch_string_8 = 'Nothing found.';
    var tipuesearch_string_9 = 'Common words are largely ignored.';
    var tipuesearch_string_10 = 'Search too short';
    var tipuesearch_string_11 = 'Should be one character or more.';
    var tipuesearch_string_12 = 'Should be';
    var tipuesearch_string_13 = 'characters or more.';
    var tipuesearch_string_14 = 'seconds';
    var tipuesearch_string_15 = 'Searches related to';
}


// Internals


// Timer for showTime

var startTimer = new Date().getTime();

