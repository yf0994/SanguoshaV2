-- this script to store the basic configuration for game program itself
-- and it is a little different from config.ini

config = {
	version = "20170831",
	version_name = "V2_EX",
	mod_name = "NONE",
	big_font = 56,
	small_font = 27,
	tiny_font = 18,
	kingdoms = { "wei", "shu", "wu", "qun", "god" },
	kingdom_colors = {
		wei = "#547998",
		shu = "#D0796C",
		wu = "#4DB873",
		qun = "#8A807A",
		god = "#96943D",
		ye = "#994AB1",
	},

	skill_type_colors = {
		compulsoryskill = "#0000FF",
		limitedskill = "#FF0000",
		wakeskill = "#800080",
		lordskill = "#FFA500",
		oppphskill = "#008000",
	},

	package_names = {
		"StandardCard",
		"StandardExCard",
		"Maneuvering",
		"LimitationBroken",
		"SPCard",
		"Nostalgia",
		"GreenHandCard",
		"New3v3Card",
		"New3v3_2013Card",
		"New1v1Card",
		"LingCards",
		"Godlailailai",

		"Standard",
		"Wind",
		"Fire",
		"Thicket",
		"Mountain",
		"God",
		"YJCM",
		"YJCM2012",
		"YJCM2013",
		"YJCM2014",
		"Assassins",
		"Special3v3",
		"Special3v3Ext",
		"Special1v1",
		"Special1v1Ext",
		"SP",
		"OL",
		"TaiwanSP",
		"Miscellaneous",
		"BGM",
		"BGMDIY",
		"Ling",
		"Hegemony",
		"HFormation",
		"HMomentum",
		"HegemonySP",
		"NostalStandard",
		"NostalWind",
		"NostalYJCM",
		"NostalYJCM2012",
		"NostalYJCM2013",
		"NostalGeneral",
		"JianGeDefense",
		"BossMode",
		"Dragon",
		"Chaos",
		"LingEx",
		"Ling2013",
		"HFormationEx",
		"HMomentumEx",
		"Test"
	},

	hulao_generals = {
		"package:nostal_standard",
		"package:wind",
		"package:nostal_wind",
		"zhenji", "zhugeliang", "sunquan", "sunshangxiang",
		"-zhangjiao", "-zhoutai", "-caoren", "-yuji",
		"-nos_yuji"
	},

	xmode_generals = {
		"package:nostal_standard",
		"package:wind",
		"package:fire",
		"package:nostal_wind",
		"zhenji", "zhugeliang", "sunquan", "sunshangxiang",
		"-nos_huatuo",
		"-zhangjiao", "-zhoutai", "-caoren", "-yuji",
		"-nos_zhangjiao", "-nos_yuji"
	},

	easy_text = {
		"太慢了，做两个俯卧撑吧！",
		"快点吧，我等的花儿都谢了！",
		"高，实在是高！",
		"好手段，可真不一般啊！",
		"哦，太菜了。水平有待提高。",
		"你会不会玩啊？！",
		"嘿，一般人，我不使这招。",
		"呵，好牌就是这么打地！",
		"杀！神挡杀神！佛挡杀佛！",
		"你也忒坏了吧？！"
	},

	roles_ban = {
	},

	kof_ban = {
		"sunquan",
	},

	bossmode_ban = {
		"caopi",
		"manchong",
		"xusheng",
		"yuji",
		"caiwenji",
		"zuoci",
		"lusu",
		"bgm_diaochan",
		"shenguanyu",
		"nos_yuji",
		"nos_zhuran"
	},

	basara_ban = {
		"dongzhuo",
		"zuoci",
		"shenzhugeliang",
		"shenlvbu",
		"bgm_lvmeng"
	},

	hegemony_ban = {
		"zuoci", "bgm_pangtong", "nos_madai", "xiahoushi", "cenhun", "shenguanyu", "shenlvmeng", "shenzhouyu", "shenzhugeliang", "shencaocao", "shenlvbu", "shencaocao", "shensimayi", "zhaoxiang", "tw_xiahouba",
		--界限突破+标准
		"caocao+xiahoudun", "nos_caocao+xiahoudun",
		"xiahoudun+zhangchunhua", "xiahoudun+nos_zhangchunhua",
		"guojia+caorui", "nos_guojia+caorui", "guojia+ol_caorui", "nos_guojia+ol_caorui", "guojia+manchong", "nos_guojia+manchong",
		"zhenji+nos_wangyi", "zhenji+caoang",
		"guanyu+guanping", "nos_guanyu+guanping", "jsp_guanyu+guanping", "sp_guanyu+guanping",
		"zhugeliang+nos_xushu",
		"sunquan+luxun",
		"nos_huanggai+zhoutai", "nos_huanggai+wuguotai",
		"sunshangxiang+liuzan",
		"luxun+guyong", "luxun+quancong",
		"nos_luxun+guyong",
		"heg_luxun+buzhi",
		"bgm_diaochan+as_fuhuanghou", 
		"st_yuanshu+nos_fuhuanghou",
		--神话再临
		"ol_xiahouyuan+caoren", "ol_xiahouyuan+nos_caoren", "ol_xiahouyuan+bgm_caoren", "ol_xiahouyuan+ol_caoren", "ol_xiahouyuan+caozhi",
		"caoren+caozhi", "ol_caoren+caozhi", "nos_caoren+caozhi", "bgm_caoren+caozhi", 
		"sunjian+dongzhuo", "sunjian+wutugu", "sunjian+zhugedan",
		"ol_weiyan+jsp_sunshangxiang",
		"wolong+zhangsong",
		"yanliangwenchou+miheng",
		"dengai+caoang",
		"liushan+nos_xushu",
		--一将成名
		"chengong+jushou",
		"liubiao+kongrong", "ol_liubiao+kongrong",
		"wangyi+zhugedan", "nos_wangyi+zhugedan", "ol_wangyi+zhugedan",
		"guyong+buzhi",
		"caorui+xizhicai", "ol_caorui+xizhicai",
		"guotupangji+liuxie",
		"sunziliufang+caoren", "sunziliufang+nos_caoren", "sunziliufang+bgm_caoren", "sunziliufang+ol_caoren", "sunziliufang+ol_xiahouyuan", "sunziliufang+caozhi", 
		--SP
		"liuxie+yuanshu",
		"yuanshu+kongrong",
		"zhugedan+diy_wangyuanji", "zhugedan+nos_zhangchunhua", "zhugedan+ol_wangyi", "zhugedan+nos_wangyi", "zhugedan+wangyi", 
	},

	pairs_ban = {
		"huatuo", "nos_huatuo", "zuoci", "bgm_pangtong", "nos_madai", "xiahoushi", "cenhun", "zhaoxiang",
		--界限突破+标准
		"caocao+xiahoudun", "nos_caocao+xiahoudun",
		"xiahoudun+luxun", "xiahoudun+nos_luxun", "xiahoudun+zhurong", "xiahoudun+zhangchunhua", "xiahoudun+nos_zhangchunhua", "xiahoudun+jiangwanfeiyi",
		"ol_xiahoudun+luxun", "ol_xiahoudun+nos_luxun", "ol_xiahoudun+zhangchunhua", "ol_xiahoudun+nos_zhangchunhua", "ol_xiahoudun+jiangwanfeiyi",
		"guojia+wolong", "nos_guojia+wolong", "guojia+caorui", "nos_guojia+caorui", "guojia+ol_caorui", "nos_guojia+ol_caorui", "guojia+wuguotai", "nos_guojia+wuguotai",
		"xizhicai+wolong", "xizhicai+caorui", "xizhicai+ol_caorui", "xizhicai+wuguotai",
		"zhenji+nos_huangyueying", "zhenji+sunshangxiang", "zhenji+nos_zhangjiao", "zhenji+zhangjiao", "zhenji+ol_zhangjiao", "zhenji+nos_wangyi", "zhenji+liuxie", "zhenji+caoang",
		"lidian+yuanshao", "ol_lidian+yuanshao", 
		"liubei+luxun", "liubei+nos_luxun", "liubei+zhangchunhua", "liubei+nos_zhangchunhua",
		"super_liubei+luxun", "super_liubei+nos_luxun", "super_liubei+zhangchunhua", "super_liubei+nos_zhangchunhua",
		"nos_liubei+luxun", "nos_liubei+nos_luxun", "nos_liubei+zhangchunhua", "nos_liubei+nos_zhangchunhua",
		"ol_liubei+luxun", "ol_liubei+nos_luxun", "ol_liubei+zhangchunhua", "ol_liubei+nos_zhangchunhua",
		"lord_liubei+luxun", "lord_liubei+nos_luxun", "lord_liubei+zhangchunhua", "lord_liubei+nos_zhangchunhua",
		"guanyu+miheng", "nos_guanyu+miheng", "jsp_guanyu+miheng", "sp_guanyu+miheng", "guansuo+miheng",
		"guanyu+guanping", "nos_guanyu+guanping", "jsp_guanyu+guanping", "sp_guanyu+guanping",
		"zhangfei+nos_huanggai", "nos_zhangfei+nos_huanggai", "zhangfei+fuwan", "nos_zhangfei+fuwan", "zhangfei+jsp_machao", "nos_zhangfei+jsp_machao",
		"xiahouba+nos_huanggai", "xiahouba+jsp_machao",
		"zhugeliang+nos_xushu",
		"huangyueying+nos_huanggai", "huangyueying+sunshangxiang", "huangyueying+liuzan", "huangyueying+yanliangwenchou",
		"nos_huangyueying+nos_huanggai", "nos_huangyueying+sunshangxiang", "nos_huangyueying+liuzan",
		"zhangsong+nos_huanggai", "zhangsong+sunshangxiang", "zhangsong+liuzan",
		"sunquan+luxun", "sunquan+kongrong",
		"lvmeng+yuanshu", "lvmeng+liubiao", "lvmeng+ol_liubiao", "lvmeng+jiangwanfeiyi",
		"nos_lvmeng+yuanshu", "nos_lvmeng+liubiao", "nos_lvmeng+ol_liubiao", "nos_lvmeng+jiangwanfeiyi",
		"bgm_lvmeng+yuanshu", "bgm_lvmeng+liubiao", "bgm_lvmeng+ol_liubiao", "bgm_lvmeng+jiangwanfeiyi",
		"huanggai+dongzhuo", "huanggai+wangyi", "huanggai+jushou", "huanggai+shenzhouyu", "huanggai+ol_weiyan",
		"nos_huanggai+st_huaxiong", "nos_huanggai+shenzhouyu", "nos_huanggai+ol_weiyan", "nos_huanggai+weiyan", "nos_huanggai+zhoutai", "nos_huanggai+dongzhuo", "nos_huanggai+yuanshao", "nos_huanggai+yanliangwenchou", "nos_huanggai+wuguotai", "nos_huanggai+guanxingzhangbao", "nos_huanggai+nos_guanxingzhangbao", "nos_huanggai+huaxiong", "nos_huanggai+xiahouba", "nos_huanggai+ol_weiyan", "nos_huanggai+wutugu", "nos_huanggai+zhuling",
		"sunshangxiang+liuzan",
		"luxun+nos_yuji", "luxun+wolong", "luxun+yanliangwenchou", "luxun+guyong", "luxun+quancong", "luxun+liuxie",
		"nos_luxun+nos_yuji", "nos_luxun+wolong", "nos_luxun+guyong", "nos_luxun+yanliangwenchou", 
		"heg_luxun+buzhi",
		"bgm_diaochan+caoren", "bgm_diaochan+nos_caoren", "bgm_diaochan+bgm_caoren", "bgm_diaochan+ol_caoren", "bgm_diaochan+caozhi", "bgm_diaochan+liaohua", "bgm_diaochan+shenlvbu", "bgm_diaochan+as_fuhuanghou", "bgm_diaochan+ol_xiahouyuan", "bgm_diaochan+guansuo",
		"st_yuanshu+nos_fuhuanghou",
		--神话再临
		"shencaocao+caozhi", "shencaocao+yanliangwenchou", "shencaocao+wolong", "shencaocao+tw_zumao", "shencaocao+tw_caoang",
		"shenzhaoyun+dongzhuo", "shenzhaoyun+heg_dongzhuo", "shenzhaoyun+huaxiong", "shenzhaoyun+st_huaxiong", "shenzhaoyun+zhugedan", "shenzhaoyun+mushun", "shenzhaoyun+wutugu",
		"shenlvbu+caoren", "shenlvbu+nos_caoren", "shenlvbu+bgm_caoren", "shenlvbu+ol_caoren", "shenlvbu+caozhi", "shenlvbu+liaohua", "shenlvbu+as_fuhuanghou", "shenlvbu+ol_xiahouyuan", "shenlvbu+guansuo",
		"ol_xiahouyuan+caoren", "ol_xiahouyuan+nos_caoren", "ol_xiahouyuan+bgm_caoren", "ol_xiahouyuan+ol_caoren", "ol_xiahouyuan+caozhi", "ol_xiahouyuan+as_fuhuanghou",
		"caoren+caozhi", "nos_caoren+caozhi", "bgm_caoren+caozhi", "ol_caoren+caozhi", "caoren+as_fuhuanghou", "nos_caoren+as_fuhuanghou", "bgm_caoren+as_fuhuanghou", "ol_caoren+as_fuhuanghou",
		"huangzhong+xusheng", "ol_huangzhong+xusheng",
		"ol_weiyan+jsp_sunshangxiang",
		"xiaoqiao+zhangchunhua", "xiaoqiao+nos_zhangchunhua", "xiaoqiao+caorui", "xiaoqiao+ol_caorui",
		"zhangjiao+dengai", "nos_zhangjiao+dengai", "ol_zhangjiao+dengai",
		"nos_yuji+zhangchunhua", "nos_yuji+nos_zhangchunhua", 
		"xunyu+wuguotai",
		"menghuo+dongzhuo", "menghuo+zhugedan", "menghuo+wutugu", "menghuo+heg_dongzhuo", "menghuo+mushun",
		"sunjian+zhugedan", 
		"dongzhuo+nos_zhangchunhua", "dongzhuo+wangyi", "dongzhuo+nos_wangyi", "dongzhuo+ol_wangyi", "dongzhuo+diy_wangyuanji",
		"heg_dongzhuo+nos_zhangchunhua", "heg_dongzhuo+wangyi", "heg_dongzhuo+nos_wangyi", "heg_dongzhuo+ol_wangyi", "heg_dongzhuo+diy_wangyuanji",
		"wolong+zhangchunhua", "wolong+nos_zhangchunhua", "wolong+zhangsong", "wolong+liuzan",
		"yuanshao+nos_zhangchunhua",
		"yanliangwenchou+miheng",
		"zhanghe+yuanshu", "zhanghe+liubiao", "zhanghe+ol_liubiao",
		"dengai+zhugejin", "dengai+caoang",
		"liushan+nos_xushu",
		--一将成名
		"xushu+yujin", "nos_xushu+yujin", "nos_xushu+kongrong",
		"chengong+jushou",
		"wuguotai+zhangchunhua", "wuguotai+nos_zhangchunhua",
		"zhangchunhua+guyong", "zhangchunhua+liuchen", "zhangchunhua+erqiao", "zhangchunhua+heg_luxun", "zhangchunhua+zhugeke", "zhangchunhua+sunhao",
		"nos_zhangchunhua+guyong", "nos_zhangchunhua+liuchen", "nos_zhangchunhua+erqiao", "nos_zhangchunhua+heg_luxun", "nos_zhangchunhua+zhugedan", "nos_zhangchunhua+zhugeke", "nos_zhangchunhua+mushun", "zhangchunhua+sunhao", "nos_zhangchunhua+wutugu",
		"liaohua+guotupangji", "liaohua+jiangwanfeiyi", "liaohua+kanze",
		"guansuo+guotupangji", "guansuo+jiangwanfeiyi", "guansuo+kanze",
		"liubiao+kongrong", "ol_liubiao+kongrong",
		"wangyi+wutugu", "wangyi+zhugedan", "wangyi+mushun",
		"nos_wangyi+wutugu", "nos_wangyi+zhugedan", "nos_wangyi+mushun",
		"ol_wangyi+wutugu", "ol_wangyi+zhugedan", "ol_wangyi+mushun",
		"guyong+buzhi",
		"nos_zhuran+hetaihou",
		"jushou+liuzan", "jushou+tw_zumao", "jushou+tw_caoang",
		"guotupangji+liuxie",
		"sunziliufang+caoren", "sunziliufang+nos_caoren", "sunziliufang+bgm_caoren", "sunziliufang+ol_caoren", "sunziliufang+ol_xiahouyuan", "sunziliufang+caozhi", "sunziliufang+bgm_diaochan", "sunziliufang+shenlvbu", "sunziliufang+as_fuhuanghou", "sunziliufang+caozhi", 
		--SP
		"liuxie+kongrong", "liuxie+yuanshu",
		"yuanshu+kongrong",
		"zhugedan+diy_wangyuanji", 
		--OL
		"wutugu+diy_wangyuanji",
		"wanglang+jianyong", "wutugu+xushi",
		"taoqian+shenzhaoyun", "taoqian+menghuo", "taoqian+sunjian", "taoqian+nos_zhangchunhua", "taoqian+diy_wangyuanji", "taoqian+wangyi", "taoqian+ol_wangyi", "taoqian+nos_wangyi",
	},

	couple_lord = "caocao",
	couple_couples = {
		"caopi|caozhi+zhenji",
		"simayi|shensimayi+zhangchunhua",
		"diy_simazhao+diy_wangyuanji",
		"liubei|bgm_liubei+ganfuren|mifuren|sp_sunshangxiang",
		"liushan+xingcai",
		"zhangfei|bgm_zhangfei+xiahoushi",
		"zhugeliang|wolong|shenzhugeliang+huangyueying",
		"menghuo+zhurong",
		"zhouyu|shenzhouyu+xiaoqiao",
		"lvbu|shenlvbu+diaochan|bgm_diaochan",
		"sunjian+wuguotai",
		"sunce|heg_sunce+daqiao|bgm_daqiao",
		"sunquan+bulianshi",
		"liuxie|diy_liuxie+fuhuanghou",
		"luxun|heg_luxun+sunru",
		"liubiao+caifuren",
	},

	convert_pairs = {
		"caiwenji->sp_caiwenji",
		"jiaxu->sp_jiaxu",
		"nos_machao->sp_machao",
	},

	-- 珠联璧合关系表
	companion_pairs = {
		-- 魏
		"caocao+xuchu|dianwei",
		"caopi+zhenji|caozhi",
		"xiahoudun+xiahouyuan",
		"caoren+caohong",
		"zhangliao+zangba",
		"xunyu+xunyou",
		"dengai+zhonghui",
		"simayi+zhangchunhua",
		"lidian+yuejin",

		-- 蜀
		"liubei+guanyu|zhangfei|ganfuren|mifuren",
		"guanyu+guanyinping",
		"zhugeliang+huangyueying",
		"wolong+pangtong|huangyueying",
		"zhaoyun+liushan|mifuren",
		"huangzhong+weiyan|fazheng",
		"menghuo+zhurong",
		"machao+madai",

		-- 吴
		"sunquan+bulianshi|zhoutai",
		"heg_sunce+daqiao|zhouyu|taishici",
		"sunjian+wuguotai",
		"zhouyu+huanggai|xiaoqiao|lusu",
		"daqiao+xiaoqiao",
		"handang+chengpu",
		"heg_xusheng+dingfeng",
		"jiangqin+zhoutai",

		-- 群
		"diaochan+lvbu|dongzhuo",
		"yuanshao+yanliangwenchou",
		"zhangjiao+zhangbao",
		"lvbu+chengong",
		"liuxie+fuhuanghou",
	},

	removed_hidden_generals = {},

	extra_hidden_generals = {
	},

	removed_default_lords = {
	},

	extra_default_lords = {
	},

	bossmode_default_boss = {
		"boss_chi+boss_mei+boss_wang+boss_liang",
		"boss_niutou+boss_mamian",
		"boss_heiwuchang+boss_baiwuchang",
		"boss_luocha+boss_yecha"
	},

	bossmode_endless_skills = {
		"bossguimei", "bossdidong", "nosenyuan", "bossshanbeng+bossbeiming+huilei+bossmingbao",
		"bossluolei", "bossguihuo", "bossbaolian", "mengjin", "bossmanjia+bazhen",
		"bossxiaoshou", "bossguiji", "fankui", "bosslianyu", "nosjuece",
		"bosstaiping+shenwei", "bosssuoming", "bossxixing", "bossqiangzheng",
		"bosszuijiu", "bossmodao", "bossqushou", "yizhong", "kuanggu",
		"bossmojian", "bossdanshu", "shenji", "wushuang", "wansha"
	},

	bossmode_exp_skills = {
		"mashu:15",
		"tannang:25",
		"yicong:25",
		"feiying:30",
		"yingyang:30",
		"zhenwei:40",
		"nosqicai:40",
		"nosyingzi:40",
		"zongshi:40",
		"qicai:45",
		"wangzun:45",
		"yingzi:50",
		"kongcheng:50",
		"nosqianxun:50",
		"weimu:50",
		"jie:50",
		"huoshou:50",
		"hongyuan:55",
		"dangxian:55",
		"xinzhan:55",
		"juxiang:55",
		"wushuang:60",
		"xunxun:60",
		"zishou:60",
		"jingce:60",
		"shengxi:60",
		"zhichi:60",
		"bazhen:60",
		"yizhong:65",
		"jieyuan:70",
		"mingshi:70",
		"tuxi:70",
		"guanxing:70",
		"juejing:75",
		"jiangchi:75",
		"bosszuijiu:80",
		"shelie:80",
		"gongxin:80",
		"fenyong:85",
		"kuanggu:85",
		"yongsi:90",
		"zhiheng:90",
	},

	jiange_defense_kingdoms = {
		loyalist = "shu",
		rebel = "wei",
	},

	jiange_defense_machine = {
		wei = "jg_machine_tuntianchiwen+jg_machine_shihuosuanni+jg_machine_fudibian+jg_machine_lieshiyazi",
		shu = "jg_machine_yunpingqinglong+jg_machine_jileibaihu+jg_machine_lingjiaxuanwu+jg_machine_chiyuzhuque",
	},

	jiange_defense_soul = {
		wei = "jg_soul_caozhen+jg_soul_simayi+jg_soul_xiahouyuan+jg_soul_zhanghe",
		shu = "jg_soul_liubei+jg_soul_zhugeliang+jg_soul_huangyueying+jg_soul_pangtong",
	},

	robot_names = {
		--[["费叔君", "常令乐", "宗政尧文", "欧阳嘉圣", "权旻",
		"轩辕伯2013", "危帅", "管利", "端木持杜", "扶杭舟",
		"第五欢楠", "井盖舒", "印潮榜", "辛起", "和珐",
		"孟坚浦", "贾引无", "公孙郁友", "南宫挺", "养和北",	]]--
		"阳光",--sunny
		"赵子龙",
		"周公瑾",
		"张伯岐",
		"钟元常",
		"無に帰ろう", --Moligaloo
		"啦啦失恋过", --啦啦SLG
		"【Pikachu】Fs", --Fsu0413
		"凌电信", --凌天翼
		"元嘉体",
		"萌豚紙",  --豚紙
		"女王神·Slob", --女王受·虫
		"DBDBD！免费哟！", --Double_Bit？
		"爱 上穹妹的 Jia", --爱上穹妹的某
		"没驾照开不了车", --开不了车
		"写书的独孤安河", --独孤安河
		"百年东南西北风", --百年东风
		"Paracel_00发", --Paracel_007
		"haveago823" , --haveatry823
		"离人泪026", --lrl026
		"墨宣砚韵", --a late developer
		"忧郁のlzxqqqq", --忧郁的月兔（lzxqqqq）
		"卍brianのvong卍", --卍冰の羽卍
		"五毛羽", --arrow羽
		"大同人陈家祺" , --陈家祺大同中心
		"叫什么啊我妹" , --你妹大神
		"麦当劳" , --果然萝卜斩
		"高调的富妮妮" , --低调的付尼玛
		"☆№Ｌ36×李Ｊ№★" , --☆№Ｌ糾×結Ｊ№★
		"老A呱呱呱", --hmqgg
		"Nagisa乐意", --Nagisa_Willing
		"0o叮咚咚叮o0", --0o叮咚Backup
		"医治曙光", --医治永恒（曙光）
		"甄姬真姬", --甄姬真妓（日月小辰）
		"tangjs我爱你", --tangjs520
		"帷幕之下问心云",
		"普肉", --Procellarum
		"大内总管KK", --KenKic
		"叶落孤舟",
		"晓月的微信", --晓月的泪痕
		"Xasy-Po-Love", --Easy-To-Love（XPL）
		"小修司V", --小休斯
		"淫" , --银
		"清风不屈一对五", --清风弄错流年
		"非凡借刀秒大魏", --非凡神力
		"高城和二", --takashiro
		"tan∠ANY", --任意角的正切
		"刘恒飞翔", --恒星飞翔
		"寂镜Jnrio",
		"人啊环境", --rahj
		"良家大少", --祝家大少
		"SB-禽受装逼佬张", --老张
		"孝弯", --孝直
		"鱼纸酱", --鱼
		"凌乱_解锁", --铃兰_杰索
		"凹凸", --ought~
		"DJ ainomelody",--Dear J ???
		"shenglove82",
		"怒喷数字", --1131561728
		"氯丙嗪", --阿米拉嗪
		"teyteym", --myetyet
		"半圆", --半缘
		"猪肉贩龙傲天", --龙傲天
		"三国成双", --三国有单
		"棺材里的怪蜀黍", --棺材
		"心变", --xxyheaven
		"这王八", --thiswangba
		"神蛟wch鸽", --wch5621628 敢达杀群：565837324
		"DR", --Demons Run
		"游客1316", --youko1316
		"龙卷风摧毁停车场", --帮忙制图的某位
		"子爷", --丫奶
		"。。。。", --？？？？
		"83贼", --16神 JOJO杀群：515214649
		"最好的坑", --坑神 学科杀群：655685619
		"ken", --ken 幻天动漫杀群：569396837
		"送命的警", --偷心的贼
	}
}