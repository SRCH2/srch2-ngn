package com.srch2;

import org.junit.*;

public class StringTest {
  
  private void testString(String test) throws NoSuchMethodException {
    SearchableString testString= new SearchableString(test);
    StringEngine e= new StringEngine();

    e.setString(testString);
    SearchableString result= e.getString();

    /* Asserts that the the results object is not the same object as the test
       input object; this is java's equivalent of comparing pointers */
    Assert.assertFalse("failure- SearchableString is the same String",
        result == testString);
    Assert.assertEquals("failure- Incorrect value",
        testString.getValue(), result.getValue());
    System.out.println(result.getValue());

  }
  @Test
  public void testJimiHendrix() throws NoSuchMethodException {
    testString("JimiHendrix");
  }
  @Test
  public void testLittlewing() throws NoSuchMethodException {
    testString("Littlewing");
  }
  @Test
  public void testMrSmithandMissSmith() throws NoSuchMethodException {
    testString("MrSmithandMissSmith");
  }
  @Test
  public void testComeTomorrowTwoMore() throws NoSuchMethodException {
    testString("ComeTomorrowTwoMore");
  }
  @Test
  public void test孟子() throws NoSuchMethodException {
    testString("孟子");
  }

  @Test
  public void test老吾老() throws NoSuchMethodException {
    testString("老吾老，以及人之老；幼吾幼，以及人之幼。");
  }
  @Test
  public void test杜甫() throws NoSuchMethodException {
    testString("杜甫");
  }
  @Test
  public void test老子() throws NoSuchMethodException {
    testString("老子");
  }
  @Test
  public void test道可道() throws NoSuchMethodException {
    testString("道可道，非常道。名可名，非常名。无名天地之始；有名万物之母。");
  }
  @Test
  public void test白居易() throws NoSuchMethodException {
    testString("白居易");
  }
  @Test
  public void test离离原() throws NoSuchMethodException {
    testString("离离原上草，一岁一枯荣。野火烧不尽，春风吹又生。");
  }
  @Test
  public void test鲁迅() throws NoSuchMethodException {
    testString("鲁迅");
  }
  @Test
  public void test横眉冷() throws NoSuchMethodException {
    testString("横眉冷对千夫指，俯首甘为孺子牛。");
  }
  @Test
  public void test孔子() throws NoSuchMethodException {
    testString("孔子");
  }
  @Test
  public void test登东山() throws NoSuchMethodException {
    testString("登东山而小鲁，登泰山而小天下。");
  }
  @Test
  public void test生() throws NoSuchMethodException {
    testString("生，亦我所欲也，义，亦我所欲也，二者不可得兼，舍生而取义者也。");
  }
  @Test
  public void test辛弃疾() throws NoSuchMethodException {
    testString("辛弃疾");
  }
  @Test
  public void testMiss李清照() throws NoSuchMethodException {
    testString("Miss李清照");
  }
  @Test
  public void test生当作people() throws NoSuchMethodException {
    testString("生当作people杰，死亦为ghost雄。");
  }
  @Test
  public void test生当() throws NoSuchMethodException {
    testString("生当作people杰，死亦为ghost雄。--Miss李清照");
  }
  @Test
  public void testEmptyString() throws NoSuchMethodException {
    testString("");
  }
  @Test
  public void test中国孔夫子() throws NoSuchMethodException {
    testString("中国孔夫子");
  }
  @Test
  public void test登东() throws NoSuchMethodException {
    testString("登东山而小鲁，登泰山而小天下。");
  }
  @Test
  public void test登西() throws NoSuchMethodException {
    testString("登西山而小鲁，登泰山而小天下。");
  }
  @Test
  public void testㄌㄧㄅㄞ() throws NoSuchMethodException {
    testString("ㄌㄧㄅㄞ");
  }
  @Test
  public void testㄇㄥㄗ() throws NoSuchMethodException {
    testString("ㄇㄥㄗ");
  }
  @Test
  public void testㄉㄨㄈㄨ() throws NoSuchMethodException {
    testString("ㄉㄨㄈㄨ");
  }
  @Test
  public void testㄌㄠㄗ() throws NoSuchMethodException {
    testString("ㄌㄠㄗ");
  }
  @Test
  public void testㄅㄞㄐㄩㄧ() throws NoSuchMethodException {
    testString("ㄅㄞㄐㄩㄧ");
  }
  @Test
  public void testㄌㄨㄒㄩㄣ() throws NoSuchMethodException {
    testString("ㄌㄨㄒㄩㄣ");
  }
  @Test
  public void testㄎㄨㄥㄗ() throws NoSuchMethodException {
    testString("ㄎㄨㄥㄗ");
  }
  @Test
  public void testㄒㄧㄣㄑㄧㄐㄧ() throws NoSuchMethodException {
    testString("ㄒㄧㄣㄑㄧㄐㄧ");
  }
  @Test
  public void testMissㄌㄧㄑㄧㄥㄓㄠ() throws NoSuchMethodException {
    testString("Missㄌㄧㄑㄧㄥㄓㄠ");
  }
  @Test
  public void testㄓㄨㄥㄍㄨㄛㄎㄨㄥㄈㄨㄗ() throws NoSuchMethodException {
    testString("ㄓㄨㄥㄍㄨㄛㄎㄨㄥㄈㄨㄗ");
  }
  @Test
  public void testマ() throws NoSuchMethodException {
    testString("マイ·ライフの日々");
  }
  @Test
  public void test闇と光() throws NoSuchMethodException {
    testString("闇と光");
  }
  @Test
  public void test自伝() throws NoSuchMethodException {
    testString("自伝");
  }
  @Test
  public void testア() throws NoSuchMethodException {
    testString("アリス·B·トクラスの自伝");
  }
  @Test
  public void testク() throws NoSuchMethodException {
    testString("クリントン自伝：マイ·ライフ");
  }
  @Test
  public void testMissNorth回帰線get() throws NoSuchMethodException {
    testString("MissNorth回帰線get");
  }
  @Test
  public void testマル() throws NoSuchMethodException {
    testString("マルコムリトル自伝、マルコムXの自伝");
  }
  @Test
  public void testマイ() throws NoSuchMethodException {
    testString("マイ·ライフ·マイ·ライフのストーリー：ヘレン·ケラーの自伝");
  }
  @Test
  public void testВиолетовиСпилбъргсасини() throws NoSuchMethodException {
    testString("ВиолетовиСпилбъргсасини");
  }
  @Test
  public void test一个不可能重复的故事() throws NoSuchMethodException {
    testString("一个不可能重复的故事");
  }
  @Test
  public void test是中国古典军事文化遗产中的璀璨瑰宝() throws NoSuchMethodException {
    testString("是中国古典军事文化遗产中的璀璨瑰宝");
  }
  @Test
  public void test是中国优秀文化传统的重要组() throws NoSuchMethodException {
    testString("是中国优秀文化传统的重要组");
  }
  @Test
  public void test农民出身的杜洛伊胆大妄为() throws NoSuchMethodException {
    testString("农民出身的杜洛伊胆大妄为");
  }
  @Test
  public void test一九三九年春的西班牙内战早已成为历史陈迹() throws NoSuchMethodException {
    testString("一九三九年春的西班牙内战早已成为历史陈迹");
  }
  @Test
  public void testㄧㄍㄜㄅㄨㄎㄜㄋㄥㄋㄨㄥㄔㄨㄥㄈㄨㄉㄜㄍㄨㄕ() throws NoSuchMethodException {
    testString("ㄧㄍㄜㄅㄨㄎㄜㄋㄥㄋㄨㄥㄔㄨㄥㄈㄨㄉㄜㄍㄨㄕ");
  }
  @Test
  public void testKrznoDržava() throws NoSuchMethodException {
    testString("KrznoDržava");
  }
  @Test
  public void testBlokiranDeep() throws NoSuchMethodException {
    testString("BlokiranDeep");
  }
  @Test
  public void testBěžnýPostal() throws NoSuchMethodException {
    testString("BěžnýPostal");
  }
  @Test
  public void testvšudewhere() throws NoSuchMethodException {
    testString("všudewhere");
  }
  @Test
  public void testDenLængstShore() throws NoSuchMethodException {
    testString("DenLængstShore");
  }
  @Test
  public void testDræbikkeensangfuglbird() throws NoSuchMethodException {
    testString("Dræbikkeensangfuglbird");
  }
  @Test
  public void testNaardevuurtoren() throws NoSuchMethodException {
    testString("Naardevuurtoren");
  }
  @Test
  public void testEendroomvanJohnBall() throws NoSuchMethodException {
    testString("EendroomvanJohnBall");
  }
  @Test
  public void testBratFarrar() throws NoSuchMethodException {
    testString("BratFarrar");
  }
  @Test
  public void testBostonians() throws NoSuchMethodException {
    testString("Bostonians");
  }
  @Test
  public void testPhilemon() throws NoSuchMethodException {
    testString("Philemon");
  }
  @Test
  public void testhepreabook() throws NoSuchMethodException {
    testString("hepreabook");
  }
  @Test
  public void testLiebling() throws NoSuchMethodException {
    testString("Liebling");
  }
  @Test
  public void testDieKunstwardesKriegesart() throws NoSuchMethodException {
    testString("DieKunstwardesKriegesart");
  }
  @Test
  public void testΡωμαίοι() throws NoSuchMethodException {
    testString("Ρωμαίοι");
  }
  @Test
  public void testΡεύματαεντηερημίαDesert() throws NoSuchMethodException {
    testString("ΡεύματαεντηερημίαDesert");
  }
  @Test
  public void testVirágzásWilderness() throws NoSuchMethodException {
    testString("VirágzásWilderness");
  }
  @Test
  public void testAzelsőésazutolsódolog() throws NoSuchMethodException {
    testString("Azelsőésazutolsódolog,");
  }
  @Test
  public void testBurungPemangsa() throws NoSuchMethodException {
    testString("BurungPemangsa");
  }
  @Test
  public void testTheBeltonEstatePemangsa() throws NoSuchMethodException {
    testString("TheBeltonEstatePemangsa");
  }
  @Test
  public void testLadifesadiGuenevere() throws NoSuchMethodException {
    testString("LadifesadiGuenevere");
  }
  @Test
  public void testilDeerslayer() throws NoSuchMethodException {
    testString("ilDeerslayer");
  }
  @Test
  public void testZENDA의죄수() throws NoSuchMethodException {
    testString("ZENDA의죄수");
  }
  @Test
  public void test어두운dark빈() throws NoSuchMethodException {
    testString("어두운dark빈");
  }
  @Test
  public void testVaijūsvaratpiedotviņai() throws NoSuchMethodException {
    testString("Vaijūsvaratpiedotviņai?");
  }
  @Test
  public void testCamilla() throws NoSuchMethodException {
    testString("Camilla");
  }
  @Test
  public void testTamsiaiTuščiaviduris() throws NoSuchMethodException {
    testString("TamsiaiTuščiaviduris");
  }
  @Test
  public void testChicotįJester() throws NoSuchMethodException {
    testString("ChicotįJester");
  }
  @Test
  public void testInnenriksMannersavamerikanerne() throws NoSuchMethodException {
    testString("InnenriksMannersavamerikanerne");
  }
  @Test
  public void testDoctorThorneså() throws NoSuchMethodException {
    testString("DoctorThorneså");
  }
}
