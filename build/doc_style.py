# -*- coding: utf-8 -*-
"""设计文档的公共样式（暖纸底 / 衬线标题 / 打印 A4）。"""

CSS = """
:root{--paper:#f7f4ed; --ink:#16150f; --dim:#6b6558; --line:#d8d2c2;
      --accent:#0f766e; --accent2:#b45309}
*{box-sizing:border-box}
html{-webkit-text-size-adjust:100%}
body{margin:0; background:var(--paper); color:var(--ink);
     font-family:"Noto Sans SC","Microsoft YaHei",system-ui,sans-serif;
     font-size:10.4pt; line-height:1.75; letter-spacing:.01em}
.wrap{max-width:760px; margin:0 auto; padding:54px 26px 90px}
h1{font-family:"Noto Serif SC",serif; font-weight:700; font-size:37pt; line-height:1.08;
   margin:14px 0 6px; letter-spacing:.01em}
h2{font-family:"Noto Serif SC",serif; font-size:16pt; margin:38px 0 10px; padding-top:14px;
   border-top:1.4px solid var(--ink)}
h2 .en{font-family:"Cascadia Mono",monospace; font-size:8.4pt; color:var(--dim);
       letter-spacing:.14em; margin-left:10px; font-weight:400}
h3{font-family:"Noto Serif SC",serif; font-size:12pt; margin:22px 0 6px}
p{margin:9px 0}
.kicker{font-family:"Cascadia Mono",monospace; font-size:8.4pt; letter-spacing:.2em;
        color:var(--dim); text-transform:uppercase}
.sub{font-size:11pt; color:var(--dim); margin:0 0 14px}
.lede{font-family:"Noto Serif SC",serif; font-size:12.2pt; line-height:1.85; margin:16px 0 0}
.kpis{display:grid; grid-template-columns:repeat(4,1fr); gap:0; margin:30px 0 8px;
      border-top:1px solid var(--line); border-bottom:1px solid var(--line)}
.kpi{padding:14px 14px 12px 0}
.kpi b{display:block; font-family:"Noto Serif SC",serif; font-size:22pt; line-height:1.15}
.kpi span{display:block; color:var(--dim); font-size:8.4pt; margin-top:2px}
.chain{display:grid; grid-template-columns:repeat(3,1fr); gap:10px; margin:16px 0 6px}
.step{border-left:2px solid var(--accent); padding:6px 0 6px 10px; background:rgba(0,0,0,.012)}
.sn{font-family:"Noto Serif SC",serif; font-size:10.4pt}
.sd{font-size:8.6pt; color:var(--dim); line-height:1.6; margin-top:2px}
figure{margin:18px 0}
figure.bleed{width:min(1040px,96vw); margin-left:calc(50% - min(520px,48vw))}
@media print{figure.bleed{width:100%; margin-left:0}}
figure svg{width:100%; height:auto; display:block}
figcaption{font-size:8.4pt; color:var(--dim); margin-top:8px; text-align:center}
table{width:100%; border-collapse:collapse; font-size:9.2pt; margin:12px 0}
th{text-align:left; font-weight:600; border-bottom:1.2px solid var(--ink); padding:6px 6px}
td{border-bottom:1px solid var(--line); padding:5px 6px; vertical-align:top}
td.ref{font-family:"Cascadia Mono",monospace; font-size:8.6pt}
td.num{font-family:"Cascadia Mono",monospace}
.calc{font-family:"Cascadia Mono",monospace; font-size:9pt; background:rgba(15,118,110,.06);
      border-left:2px solid var(--accent); padding:8px 10px; margin:10px 0; line-height:1.7}
.calc .r{color:var(--accent); font-weight:600}
.callout{border-left:2px solid var(--accent2); background:rgba(180,83,9,.05);
         padding:9px 12px; margin:14px 0; font-size:9.6pt}
ul{margin:8px 0 8px 18px; padding:0}
li{margin:5px 0}
.foot{margin-top:46px; padding-top:12px; border-top:1px solid var(--line);
      color:var(--dim); font-size:8.4pt; line-height:1.7}
code{font-family:"Cascadia Mono",monospace; font-size:9pt;
     background:rgba(0,0,0,.05); padding:1px 4px}
@page{size:A4; margin:15mm 14mm 16mm}
@page wide{size:A4 landscape; margin:12mm 14mm}
.wide{page:wide}
@media print{.wide figure.bleed{width:100%; margin-left:0}}
@media print{
  .wrap{max-width:none; padding:0}
  h2{break-after:avoid}
  figure,table,.calc,.callout,.chain,.kpis{break-inside:avoid}
  .foot{break-inside:avoid}
}
"""
