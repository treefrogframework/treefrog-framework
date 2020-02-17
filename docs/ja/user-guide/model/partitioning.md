---
title: パーティショニング
page_id: "060.040"
---

## パーティショニング

ここでのパーティショニングとは、テーブルAとテーブルBを別のサーバに置き、DBの負荷を分散することです。Webシステムにおいて、DBがボトルネックになるケースは多いものです。

テーブルのデータ量がDBサーバのメモリに載るサイズでなくなってくると、ディスクI/Oが増えるのでパフォーマンスは相当落ちます。この軽減策としてメモリの増設ができれば簡単な話ですが（近頃メモリは安いですからね）、そんな簡単に増設はできないでしょう。

もしDBサーバ自体をを増やすことが可能ならば、パーティショニングは検討の余地があります。DBをどのようにスケールさせるかはなかなか難しい問題であり、他のネットの記事や出版されている本を参考にするのが良いと思いますのでここでは省略します。

パーティショニングのために、TreeFrog Framework には異なるDBサーバ上のデータへ簡単にアクセスする仕組みがあります。

## SqlObjectのパーティショニング

前提条件として、テーブルAはホストAに、テーブルBはホストBにあるとします。

まず、ホスト接続情報を記述したデータベース設定ファイルをそれぞれ別に作ります。ファイル名はそれぞれ databaseA.ini と databaseB.ini にし（別の名前でも構いません）、config ディレクトリに置きましょう。ファイルの内容には、それぞれ適切な値を記述します。この例ではdevセクションに定義してみました。

```ini
[dev]
DriverType=QMYSQL
DatabaseName=foodb
HostName=192.168.xxx.xxx
Port=3306
UserName=root
Password=xxxx
ConnectOptions=
```

次のこれらデータベース設定ファイルのファイル名をアプリケーション設定ファイル（application.ini）に定義します。DatabaseSettingsFiles の値に、スペース区切りで並べて書きます。

```ini
# Specify setting files for databases.
DatabaseSettingsFiles=databaseA.ini  databaseB.ini
```

データベースIDはこの並び順に０，１，．．．と振られます。つまり、この例では

* ホストAのデータベースID： 0
* ホストBのデータベースID： 1

というように割り振られます。

次に、このデータベースIDを SqlObject のヘッダファイルに設定します。<br>
ジェネレータで生成されたヘッダファイルを編集し、databaseId() メソッドをオーバライドして、このデータベースIDを返します。

```c++
class T_MODEL_EXPORT BlogObject : public TSqlObject, public QSharedData
{
      :
    int databaseId() const { return 1; }  // ID:1を返却
      :
};
```

こうすることで、BlogObject に関するクエリは常にホストBへ発行されることになります。あとは、これまで通り SqlObject  を使うだけです。

ここでの例は２つのDBサーバに対する内容でしたが、３つ以上のDBサーバにも対応しています。DatabaseSettingsFiles に設定ファイルを追加するだけです。

このように、コントローラやモデルのロジックを変更することなく、パーティショニングが適用できるのです。

## パーティショニングされたテーブルへのクエリ発行

[SQLクエリ]({{ site.baseurl }}/ja/user-guide/model/sql-query.html){:target="_blank"}の章で説明したとおり、独自にクエリを発行するには TSqlQuery オブジェクトを使用するわけですが、これにもパーティショニングを適用することができます。

次のように、コンストラクタの第２引数でデータベースIDを指定します。

```c++
TSqlQuery query("SELECT * FROM foo WHERE ...",  1);  // ID:1を指定
query.exec();
  :
```

このクエリはホストBに対して発行されます。