#include "story/dialogue_data.h"
#include "npc/npc_dialogue.h"
#include "npc/world_event.h"
#include <string.h>

/* Helper to add a dialogue node */
static DialogueNode *add_node(DialogueSystem *ds, int id, int npc_id, const char *text)
{
    if (ds->node_count >= MAX_DIALOGUE_NODES)
        return NULL;

    DialogueNode *node = &ds->nodes[ds->node_count++];
    memset(node, 0, sizeof(*node));
    node->id = id;
    node->npc_id = npc_id;
    strncpy(node->text, text, MAX_DIALOGUE_TEXT - 1);
    return node;
}

/* Helper to add a choice to a node */
static DialogueChoice *add_choice(DialogueNode *node, const char *text, int next_node_id)
{
    if (node->choice_count >= MAX_CHOICES_PER_NODE)
        return NULL;

    DialogueChoice *choice = &node->choices[node->choice_count++];
    memset(choice, 0, sizeof(*choice));
    strncpy(choice->text, text, MAX_CHOICE_TEXT - 1);
    choice->next_node_id = next_node_id;
    return choice;
}

/* Helper to add a consequence to a choice */
static void add_consequence(DialogueChoice *choice, ConsequenceType type,
                            int p1, int p2, float fp)
{
    if (choice->consequence_count >= MAX_CONSEQUENCES)
        return;

    DialogueConsequence *c = &choice->consequences[choice->consequence_count++];
    c->type = type;
    c->param1 = p1;
    c->param2 = p2;
    c->fparam = fp;
}

/* Helper to add a condition to a choice */
static void add_choice_condition(DialogueChoice *choice, ConditionType type,
                                 int p1, int p2, float fp)
{
    if (choice->condition_count >= MAX_CONDITIONS)
        return;

    DialogueCondition *cond = &choice->conditions[choice->condition_count++];
    cond->type = type;
    cond->param1 = p1;
    cond->param2 = p2;
    cond->fparam = fp;
}

/* Helper to add a condition to a node */
static void add_node_condition(DialogueNode *node, ConditionType type,
                               int p1, int p2, float fp)
{
    if (node->condition_count >= MAX_CONDITIONS)
        return;

    DialogueCondition *cond = &node->conditions[node->condition_count++];
    cond->type = type;
    cond->param1 = p1;
    cond->param2 = p2;
    cond->fparam = fp;
}

/* ===== NPC_ID_GRISWOLD (0) — Nodes 0-99 ===== */
static void init_griswold(DialogueSystem *ds)
{
    DialogueNode *n;
    DialogueChoice *c;

    /* Greeting */
    n = add_node(ds, 0, NPC_ID_GRISWOLD,
        "Another warrior... I've seen too many of your kind never return.");
    add_choice(n, "What weapons do you have?", 1);
    add_choice(n, "Tell me about the cathedral.", 5);
    add_choice(n, "What do you know about the others in town?", 8);
    add_choice(n, "Farewell.", -1);

    /* Path A: Weapons & forging */
    n = add_node(ds, 1, NPC_ID_GRISWOLD,
        "I forge what I can, but the good ore... it's all deep below now. "
        "The mines beneath the cathedral hold veins of star-metal.");
    add_choice(n, "Star-metal? Tell me more.", 2);
    add_choice(n, "I'll keep an eye out down there.", 3);
    add_choice(n, "Back to other matters.", 0);

    n = add_node(ds, 2, NPC_ID_GRISWOLD,
        "Aye, ore that fell from the sky in ages past. My grandfather forged "
        "a blade from it once. Cut through bone like parchment.");
    c = add_choice(n, "I'll find some for you.", 3);
    add_consequence(c, CONSEQ_SET_FLAG, FLAG_TALKED_GRISWOLD_FORGE, 0, 0);
    add_choice(n, "Sounds dangerous.", 4);

    n = add_node(ds, 3, NPC_ID_GRISWOLD,
        "You'd do that? Bring me star-metal ore, and I'll forge you "
        "something worthy of legend. You have my word.");
    c = add_choice(n, "Consider it done.", -1);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_GRISWOLD, 0, 0.1f);
    add_consequence(c, CONSEQ_WORLD_EVENT, EVT_PLAYER_HELPED_NPC, NPC_ID_GRISWOLD, 0.3f);

    n = add_node(ds, 4, NPC_ID_GRISWOLD,
        "Everything worth having is dangerous, friend. That's just the way "
        "of things since the cathedral fell.");
    add_choice(n, "You're right. I'll look for it.", 3);
    add_choice(n, "Maybe another time.", -1);

    /* Path B: The cathedral */
    n = add_node(ds, 5, NPC_ID_GRISWOLD,
        "That place was holy once. Archbishop Lazarus led his flock down "
        "into those depths... and something else came back wearing his face.");
    add_choice(n, "What happened to the people?", 6);
    add_choice(n, "Lazarus... I'll remember that name.", 7);

    n = add_node(ds, 6, NPC_ID_GRISWOLD,
        "Gone. Dead. Worse than dead, some say. I hear things at night, "
        "hammering at metal that doesn't exist. Echoes from below.");
    add_choice(n, "I'll put an end to it.", -1);
    add_choice(n, "Back to other matters.", 0);

    n = add_node(ds, 7, NPC_ID_GRISWOLD,
        "Remember it well. If you find him down there, don't trust a "
        "word he says. Not one word.");
    c = add_choice(n, "Understood.", -1);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_GRISWOLD, 0, 0.05f);

    /* Path C: Townspeople */
    n = add_node(ds, 8, NPC_ID_GRISWOLD,
        "Wirt? That boy charges three times what anything's worth. "
        "Don't trust his prices. Cain, though... Cain knows things. "
        "Listen to that old man.");
    add_choice(n, "What about the healer?", 9);
    add_choice(n, "And the witch?", 10);
    add_choice(n, "Thanks for the advice.", -1);

    n = add_node(ds, 9, NPC_ID_GRISWOLD,
        "Pepin's a good man. Keeps us patched together. He's worried "
        "about some sickness spreading through the well water, though.");
    add_choice(n, "I should speak with him.", -1);
    add_choice(n, "Back to other matters.", 0);

    n = add_node(ds, 10, NPC_ID_GRISWOLD,
        "Adria keeps to herself by the river. Sells potions and scrolls. "
        "I don't trust magic, but... she hasn't steered anyone wrong. Yet.");
    add_choice(n, "Good to know.", -1);
    add_choice(n, "Back to other matters.", 0);
}

/* ===== NPC_ID_PEPIN (1) — Nodes 100-199 ===== */
static void init_pepin(DialogueSystem *ds)
{
    DialogueNode *n;
    DialogueChoice *c;

    /* Greeting */
    n = add_node(ds, 100, NPC_ID_PEPIN,
        "Be careful, friend. The sickness spreads from beneath. "
        "Let me know if you need tending.");
    add_choice(n, "I could use some healing.", 101);
    add_choice(n, "What sickness? The well?", 103);
    add_choice(n, "What do you think of Adria?", 106);
    add_choice(n, "I'll be on my way.", -1);

    /* Path A: Healing */
    n = add_node(ds, 101, NPC_ID_PEPIN,
        "Hold still... let me see those wounds.");
    c = add_choice(n, "[Accept healing]", 102);
    add_consequence(c, CONSEQ_WORLD_EVENT, EVT_PLAYER_HELPED_NPC, NPC_ID_PEPIN, 0.2f);
    add_consequence(c, CONSEQ_MODIFY_MOOD, NPC_ID_PEPIN, 0, 0.1f);
    add_choice(n, "Actually, I'm fine.", -1);

    n = add_node(ds, 102, NPC_ID_PEPIN,
        "There. The herbs do their work, but they grow scarcer by the day. "
        "The corruption below poisons even the soil.");
    add_choice(n, "Thank you, Pepin.", -1);
    add_choice(n, "I have other questions.", 100);

    /* Path B: Poisoned well */
    n = add_node(ds, 103, NPC_ID_PEPIN,
        "The well water has turned foul. People fall ill, children worst "
        "of all. Something deep in the earth taints the source.");
    add_choice(n, "Can I help?", 104);
    add_choice(n, "That sounds terrible.", 105);

    n = add_node(ds, 104, NPC_ID_PEPIN,
        "If you could descend and find what corrupts the water... "
        "I've narrowed it to the third level. Some dark growth, perhaps, "
        "or a cursed artifact poisoning the aquifer.");
    c = add_choice(n, "I'll purify the source.", -1);
    add_consequence(c, CONSEQ_SET_FLAG, FLAG_POISONED_WELL_STARTED, 0, 0);
    add_consequence(c, CONSEQ_START_QUEST, FLAG_POISONED_WELL_STARTED, 0, 0);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_PEPIN, 0, 0.15f);
    add_consequence(c, CONSEQ_WORLD_EVENT, EVT_PLAYER_HELPED_NPC, NPC_ID_PEPIN, 0.4f);
    c = add_choice(n, "I'll think about it.", -1);

    n = add_node(ds, 105, NPC_ID_PEPIN,
        "Terrible doesn't begin to describe it. Little Theo is brave, "
        "but even he's been coughing. We need help.");
    add_choice(n, "I'll look into the well.", 104);
    add_choice(n, "I'm sorry.", -1);

    /* Path C: About Adria */
    n = add_node(ds, 106, NPC_ID_PEPIN,
        "Adria... she knows things no mortal should. Her remedies work, "
        "I'll grant that, but at what cost? I've seen her reading from "
        "books bound in skin that isn't leather.");
    add_choice(n, "You think she's dangerous?", 107);
    add_choice(n, "Back to other matters.", 100);

    n = add_node(ds, 107, NPC_ID_PEPIN,
        "I think power always has a price. And I think she's been paying "
        "it for a very long time. Watch yourself around her.");
    add_choice(n, "I'll be careful.", -1);
    add_choice(n, "I have other questions.", 100);
}

/* ===== NPC_ID_CAIN (2) — Nodes 200-299 ===== */
static void init_cain(DialogueSystem *ds)
{
    DialogueNode *n;
    DialogueChoice *c;

    /* Greeting */
    n = add_node(ds, 200, NPC_ID_CAIN,
        "Stay a while and listen... I have much to tell you, "
        "if you have the patience to hear it.");
    add_choice(n, "Tell me about the dungeon below.", 201);
    add_choice(n, "What is Tristram's history?", 204);
    add_choice(n, "I need guidance.", 207);
    add_choice(n, "Another time, Cain.", -1);

    /* Path A: The dungeon / Diablo lore */
    n = add_node(ds, 201, NPC_ID_CAIN,
        "Beneath the cathedral lies a labyrinth older than Tristram itself. "
        "It was built to contain one of the Prime Evils... Diablo, "
        "the Lord of Terror.");
    add_choice(n, "A Prime Evil? Explain.", 202);
    add_choice(n, "How do I stop it?", 203);

    n = add_node(ds, 202, NPC_ID_CAIN,
        "Three brothers rule Hell: Diablo, Mephisto, Baal. They were "
        "banished to our world by the Horadrim and sealed in soulstones. "
        "But seals weaken. Evil endures.");
    c = add_choice(n, "The soulstone... where is it?", 203);
    add_consequence(c, CONSEQ_SET_FLAG, FLAG_TALKED_CAIN_LORE, 0, 0);
    add_choice(n, "This is grave news.", -1);

    n = add_node(ds, 203, NPC_ID_CAIN,
        "You must descend through sixteen levels of corruption. Each "
        "deeper than the last. Find the soulstone. Destroy it if you can. "
        "But beware... Diablo will try to claim you.");
    c = add_choice(n, "I won't fail.", -1);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_CAIN, 0, 0.1f);
    add_consequence(c, CONSEQ_GIVE_XP, 25, 0, 0);
    add_choice(n, "I need to prepare first.", -1);

    /* Path B: Tristram's history */
    n = add_node(ds, 204, NPC_ID_CAIN,
        "Tristram was once a prosperous village. Merchants, farmers, "
        "families... all living in the shadow of the cathedral, "
        "never knowing what slept below.");
    add_choice(n, "What changed?", 205);
    add_choice(n, "Back to other matters.", 200);

    n = add_node(ds, 205, NPC_ID_CAIN,
        "King Leoric came. He made the cathedral his seat of power. "
        "Then the whispers began. Leoric went mad. His son, Prince Albrecht, "
        "vanished into the depths.");
    add_choice(n, "The prince... is he still alive?", 206);
    add_choice(n, "A king driven mad...", -1);

    n = add_node(ds, 206, NPC_ID_CAIN,
        "I pray he is not. For if Diablo has taken his body as a vessel... "
        "then the boy you find will not be the boy who was lost. "
        "Steel your heart for what lies below.");
    add_choice(n, "I understand.", -1);
    add_choice(n, "I have more questions.", 200);

    /* Path C: Guidance (varies by implied progression) */
    n = add_node(ds, 207, NPC_ID_CAIN,
        "You are brave to face what lies below. Let me offer what "
        "wisdom I can. The upper levels crawl with the undead. "
        "Fire and holy water will serve you well.");
    add_choice(n, "What about deeper levels?", 208);
    add_choice(n, "Thank you for the advice.", -1);

    n = add_node(ds, 208, NPC_ID_CAIN,
        "Below the catacombs, you'll find caves teeming with demons. "
        "They fear nothing mortal. You will need enchanted arms, "
        "and allies — even reluctant ones.");
    c = add_choice(n, "Allies? Who would help?", 209);
    add_choice(n, "I'll manage on my own.", -1);

    n = add_node(ds, 209, NPC_ID_CAIN,
        "Griswold can forge if given proper ore. Adria has power, though "
        "her price may be steep. Even Farnham, in his cups, knows "
        "secrets from when he ventured below.");
    c = add_choice(n, "I'll seek them out.", -1);
    add_consequence(c, CONSEQ_GIVE_XP, 10, 0, 0);
}

/* ===== NPC_ID_OGDEN (3) — Nodes 300-399 ===== */
static void init_ogden(DialogueSystem *ds)
{
    DialogueNode *n;
    DialogueChoice *c;

    /* Greeting */
    n = add_node(ds, 300, NPC_ID_OGDEN,
        "Welcome, welcome! What'll it be? A drink? A room? "
        "Or just someone to talk to?");
    add_choice(n, "Heard any rumors lately?", 301);
    add_choice(n, "How's Farnham doing?", 304);
    add_choice(n, "How's business?", 306);
    add_choice(n, "Just passing through.", -1);

    /* Path A: Rumors / gossip */
    n = add_node(ds, 301, NPC_ID_OGDEN,
        "Rumors? Ha! This town runs on them. Let me think... "
        "Griswold's been hammering all night again. Says he's working "
        "on something special.");
    add_choice(n, "What else?", 302);
    add_choice(n, "Interesting.", -1);

    n = add_node(ds, 302, NPC_ID_OGDEN,
        "Old Cain was muttering about stars aligning. And Adria... "
        "she bought every candle I had. Every last one. "
        "Whatever she's doing, it needs a lot of light.");
    add_choice(n, "Anything about the cathedral?", 303);
    add_choice(n, "Thanks for the gossip.", -1);

    n = add_node(ds, 303, NPC_ID_OGDEN,
        "Folk say the ground shakes at night near the entrance. "
        "And there's a smell... like copper and rot. Whatever's down "
        "there is getting restless.");
    c = add_choice(n, "I'll look into it.", -1);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_OGDEN, 0, 0.05f);
    add_choice(n, "Back to other matters.", 300);

    /* Path B: Farnham */
    n = add_node(ds, 304, NPC_ID_OGDEN,
        "Poor soul. He was a brave man once, went into the labyrinth "
        "with Lazarus and the others. Came back... broken. "
        "Now he just drinks to forget.");
    add_choice(n, "What did he see down there?", 305);
    add_choice(n, "That's sad.", -1);

    n = add_node(ds, 305, NPC_ID_OGDEN,
        "He won't say. Not sober, anyway. Buy him enough ale and he "
        "mumbles about 'the butcher' and rooms full of bodies. "
        "Then he screams and passes out.");
    c = add_choice(n, "The Butcher...", -1);
    add_consequence(c, CONSEQ_WORLD_EVENT, EVT_GOSSIP_BAD, NPC_ID_FARNHAM, 0.2f);
    add_choice(n, "Back to other matters.", 300);

    /* Path C: Business */
    n = add_node(ds, 306, NPC_ID_OGDEN,
        "Business? What business? Nobody travels to Tristram anymore. "
        "The trade roads are crawling with the risen dead. "
        "I'm pouring drinks for ghosts and heroes.");
    add_choice(n, "I'll clear those roads.", 307);
    add_choice(n, "Tough times.", -1);

    n = add_node(ds, 307, NPC_ID_OGDEN,
        "Would you? The east road used to connect us to Westmarch. "
        "If even a few merchants could get through... "
        "it would mean everything.");
    c = add_choice(n, "I'll see what I can do.", -1);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_OGDEN, 0, 0.1f);
    add_consequence(c, CONSEQ_MODIFY_MOOD, NPC_ID_OGDEN, 0, 0.15f);
    add_consequence(c, CONSEQ_WORLD_EVENT, EVT_PLAYER_HELPED_NPC, NPC_ID_OGDEN, 0.3f);
}

/* ===== NPC_ID_FARNHAM (4) — Nodes 400-499 ===== */
static void init_farnham(DialogueSystem *ds)
{
    DialogueNode *n;
    DialogueChoice *c;

    /* Greeting — default (low trust) */
    n = add_node(ds, 400, NPC_ID_FARNHAM,
        "Leave me... *hic*... alone. I don't want to remember.");
    c = add_choice(n, "Let me buy you a drink.", 401);
    add_choice(n, "What happened to you?", 403);
    c = add_choice(n, "I heard you know about the deeper levels.", 410);
    add_choice_condition(c, COND_FLAG_SET, FLAG_FARNHAM_TRUSTED, 0, 0);
    add_choice(n, "[Walk away]", -1);

    /* Buy drink — trust gate */
    n = add_node(ds, 401, NPC_ID_FARNHAM,
        "*takes the mug with shaking hands* ...You're not like the others. "
        "They all stare. Judge. You... you just offer a drink.");
    c = add_choice(n, "Everyone needs a friend.", 402);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_FARNHAM, 0, 0.2f);
    add_consequence(c, CONSEQ_SET_FLAG, FLAG_BOUGHT_FARNHAM_DRINK, 0, 0);
    add_consequence(c, CONSEQ_WORLD_EVENT, EVT_PLAYER_HELPED_NPC, NPC_ID_FARNHAM, 0.3f);
    c = add_choice(n, "Just drink up.", -1);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_FARNHAM, 0, 0.1f);

    n = add_node(ds, 402, NPC_ID_FARNHAM,
        "*sniffles* Friend... I had friends once. Went down with Lazarus. "
        "Good men. Strong men. None came back whole. "
        "Ask me again sometime... when the ale's done its work.");
    c = add_choice(n, "I'll come back.", -1);
    add_consequence(c, CONSEQ_SET_FLAG, FLAG_FARNHAM_TRUSTED, 0, 0);

    /* What happened — surfaces trauma */
    n = add_node(ds, 403, NPC_ID_FARNHAM,
        "*eyes go wide* Don't... don't ask me that. There was blood. "
        "So much blood. And he... IT... was waiting for us.");
    add_choice(n, "Who was waiting?", 404);
    add_choice(n, "I'm sorry. I'll leave you be.", -1);

    n = add_node(ds, 404, NPC_ID_FARNHAM,
        "THE BUTCHER! *slams fist on table* A thing of meat and fury! "
        "It wore... it wore the skins of... *breaks down sobbing*");
    c = add_choice(n, "I'll kill it. For you and your friends.", 405);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_FARNHAM, 0, 0.15f);
    add_choice(n, "[Let him cry]", -1);

    n = add_node(ds, 405, NPC_ID_FARNHAM,
        "*looks up with bloodshot eyes* You mean it? Third level down. "
        "There's a room... you'll know it by the hooks. "
        "And the smell. God, the smell.");
    c = add_choice(n, "It ends now.", -1);
    add_consequence(c, CONSEQ_GIVE_XP, 15, 0, 0);
    add_consequence(c, CONSEQ_WORLD_EVENT, EVT_GOSSIP_GOOD, NPC_ID_FARNHAM, 0.3f);

    /* Deeper conversation — requires trust (condition on greeting node variant) */
    n = add_node(ds, 410, NPC_ID_FARNHAM,
        "*steadier now* You came back. Not many do that. "
        "Sit down. I'll tell you what I know.");
    add_node_condition(n, COND_FLAG_SET, FLAG_FARNHAM_TRUSTED, 0, 0);
    add_choice(n, "Tell me about the deeper levels.", 411);
    add_choice(n, "What else lurks below?", 412);
    add_choice(n, "How are you holding up?", -1);

    n = add_node(ds, 411, NPC_ID_FARNHAM,
        "Below the Butcher... caves. Dark ones. We heard chanting. "
        "Lazarus was leading us like lambs. I think... I think he knew "
        "what was down there all along.");
    c = add_choice(n, "Lazarus betrayed you.", -1);
    add_consequence(c, CONSEQ_WORLD_EVENT, EVT_GOSSIP_BAD, -1, 0.4f);

    n = add_node(ds, 412, NPC_ID_FARNHAM,
        "There's something in the dark that breathes. Not like us. "
        "Deep, slow breaths that shake the walls. Whatever it is, "
        "it's been sleeping a long time. And it's waking up.");
    c = add_choice(n, "Diablo...", -1);
    add_consequence(c, CONSEQ_GIVE_XP, 20, 0, 0);
}

/* ===== NPC_ID_ADRIA (5) — Nodes 500-599 ===== */
static void init_adria(DialogueSystem *ds)
{
    DialogueNode *n;
    DialogueChoice *c;

    /* Greeting — default */
    n = add_node(ds, 500, NPC_ID_ADRIA,
        "I sense... power in you. Raw. Untamed. "
        "Perhaps we can be of use to each other.");
    add_choice(n, "Tell me about your magic.", 501);
    add_choice(n, "What do you know about the dungeon?", 504);
    add_choice(n, "Not interested.", -1);

    /* Path A: Magic — witch's bargain */
    n = add_node(ds, 501, NPC_ID_ADRIA,
        "Magic is merely will given form. I can teach you to shape it... "
        "but knowledge demands sacrifice. Always.");
    add_choice(n, "What kind of sacrifice?", 502);
    add_choice(n, "I'll stick to my sword.", -1);

    n = add_node(ds, 502, NPC_ID_ADRIA,
        "A fragment of your essence. Life force. Nothing you'd miss... "
        "at first. In return, I grant you sight beyond sight. "
        "The ability to sense evil before it strikes.");
    c = add_choice(n, "[Accept the bargain]", 503);
    add_consequence(c, CONSEQ_SET_FLAG, FLAG_WITCHS_BARGAIN_DONE, 0, 0);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_ADRIA, 0, 0.2f);
    add_consequence(c, CONSEQ_GIVE_XP, 50, 0, 0);
    add_consequence(c, CONSEQ_WORLD_EVENT, EVT_GOSSIP_BAD, NPC_ID_ADRIA, 0.5f);
    c = add_choice(n, "[Refuse]", -1);
    add_consequence(c, CONSEQ_MODIFY_MOOD, NPC_ID_ADRIA, 0, -0.1f);

    n = add_node(ds, 503, NPC_ID_ADRIA,
        "*her eyes glow briefly* It is done. You will feel the shadows "
        "differently now. They have texture. Weight. Be wary of "
        "what you learn to see.");
    add_choice(n, "What have you done to me?", -1);
    add_choice(n, "Thank you... I think.", -1);

    /* Path B: Dungeon knowledge */
    n = add_node(ds, 504, NPC_ID_ADRIA,
        "The labyrinth breathes. It changes. Corridors shift when no one "
        "watches. This is not architecture — it is alive.");
    add_choice(n, "How do I navigate it?", 505);
    add_choice(n, "That's... unsettling.", -1);

    n = add_node(ds, 505, NPC_ID_ADRIA,
        "Follow the cold. Evil radiates cold the way a fire radiates heat. "
        "The deeper the chill, the closer you are to the source. "
        "And when you feel warmth again... run.");
    add_choice(n, "Why warmth?", 506);
    add_choice(n, "I'll remember that.", -1);

    n = add_node(ds, 506, NPC_ID_ADRIA,
        "Because warmth means it's behind you. Already watching. "
        "Already hungry.");
    add_choice(n, "...", -1);

    /* Post-bargain greeting variant */
    n = add_node(ds, 510, NPC_ID_ADRIA,
        "Ah, you return. Tell me... have the shadows spoken to you yet? "
        "They will. Give them time.");
    add_node_condition(n, COND_FLAG_SET, FLAG_WITCHS_BARGAIN_DONE, 0, 0);
    add_choice(n, "What else can you teach me?", 511);
    add_choice(n, "The shadows show me nothing.", -1);

    n = add_node(ds, 511, NPC_ID_ADRIA,
        "Patience. The gift unfolds in layers. But I can offer you "
        "another lesson... for another price. There is always another price.");
    add_choice(n, "Name it.", -1);
    add_choice(n, "One bargain was enough.", -1);
}

/* ===== NPC_ID_THEO (6) — Nodes 600-699 ===== */
static void init_theo(DialogueSystem *ds)
{
    DialogueNode *n;
    DialogueChoice *c;

    /* Greeting */
    n = add_node(ds, 600, NPC_ID_THEO,
        "Hey! Are you a real hero?! You have a sword and everything!");
    add_choice(n, "I try to be.", 601);
    add_choice(n, "Go home, kid. It's dangerous.", 604);
    add_choice(n, "...", -1);

    /* Path A: Tell about adventures */
    n = add_node(ds, 601, NPC_ID_THEO,
        "*eyes wide* Really?! Have you been in the cathedral? "
        "Have you fought monsters? Tell me everything!");
    add_choice(n, "I've fought things you wouldn't believe.", 602);
    add_choice(n, "It's not as exciting as you think.", 603);

    n = add_node(ds, 602, NPC_ID_THEO,
        "WHOA! I knew it! I want to be a hero too! I've been practicing "
        "with a stick. I can show you the secret passages I found "
        "near the old well!");
    c = add_choice(n, "Show me what you've found.", 603);
    add_consequence(c, CONSEQ_SET_FLAG, FLAG_THEOS_ADVENTURE, 0, 0);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_THEO, 0, 0.3f);
    add_consequence(c, CONSEQ_WORLD_EVENT, EVT_PLAYER_HELPED_NPC, NPC_ID_THEO, 0.2f);
    c = add_choice(n, "Stay safe, Theo. Leave the fighting to me.", -1);
    add_consequence(c, CONSEQ_MODIFY_MOOD, NPC_ID_THEO, 0, -0.1f);

    n = add_node(ds, 603, NPC_ID_THEO,
        "I found a crack in the wall near the old well. There's a tunnel "
        "behind it! I was too scared to go in alone, but with you...");
    c = add_choice(n, "Let's check it out together — carefully.", -1);
    add_consequence(c, CONSEQ_SET_FLAG, FLAG_THEOS_ADVENTURE, 0, 0);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_THEO, 0, 0.2f);
    add_consequence(c, CONSEQ_GIVE_XP, 10, 0, 0);
    c = add_choice(n, "You should tell Pepin about that tunnel.", -1);

    /* Path B: Tell him to stay home */
    n = add_node(ds, 604, NPC_ID_THEO,
        "*shoulders slump* Everyone says that... Pepin says that, "
        "Ogden says that. But someone has to do something! "
        "The monsters are getting closer!");
    c = add_choice(n, "You're braver than you know. But stay safe.", -1);
    add_consequence(c, CONSEQ_MODIFY_MOOD, NPC_ID_THEO, 0, 0.05f);
    c = add_choice(n, "Fine. What do you know?", 601);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_THEO, 0, 0.1f);
}

/* ===== NPC_ID_GHARBAD (7) — Nodes 700-799 ===== */
static void init_gharbad(DialogueSystem *ds)
{
    DialogueNode *n;
    DialogueChoice *c;

    /* Greeting */
    n = add_node(ds, 700, NPC_ID_GHARBAD,
        "Please... don't hurt Gharbad. Gharbad is not like the others. "
        "Gharbad only wants to live in peace.");
    add_choice(n, "I won't hurt you. What are you?", 701);
    add_choice(n, "Give me one reason not to.", 705);
    add_choice(n, "[Leave him alone]", -1);

    /* Path A: Be kind — Gharbad's Plea */
    n = add_node(ds, 701, NPC_ID_GHARBAD,
        "Gharbad was a shaman. Small tribe. The demons came and took "
        "Gharbad's people. Made them fight. Made them eat things. "
        "Gharbad ran. Gharbad is coward, yes... but alive.");
    add_choice(n, "You're not a coward. You're a survivor.", 702);
    add_choice(n, "Can you help me fight the demons?", 703);

    n = add_node(ds, 702, NPC_ID_GHARBAD,
        "*trembles* Survivor... yes. Gharbad likes that word. "
        "Gharbad can help! Gharbad knows things. Knows where the demons "
        "keep their treasures. Help Gharbad, and Gharbad helps you.");
    c = add_choice(n, "It's a deal, Gharbad.", 704);
    add_consequence(c, CONSEQ_SET_FLAG, FLAG_GHARBADS_PLEA, 0, 0);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_GHARBAD, 0, 0.3f);
    add_consequence(c, CONSEQ_WORLD_EVENT, EVT_PLAYER_HELPED_NPC, NPC_ID_GHARBAD, 0.4f);
    add_choice(n, "I'll think about it.", -1);

    n = add_node(ds, 703, NPC_ID_GHARBAD,
        "Fight?! Gharbad is small! Gharbad is weak! But... Gharbad "
        "knows herbs. Poisons. Gharbad can make things that hurt demons. "
        "If you protect Gharbad.");
    c = add_choice(n, "You make the potions, I'll do the fighting.", 704);
    add_consequence(c, CONSEQ_SET_FLAG, FLAG_GHARBADS_PLEA, 0, 0);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_GHARBAD, 0, 0.25f);
    add_choice(n, "We'll see.", -1);

    n = add_node(ds, 704, NPC_ID_GHARBAD,
        "*nods rapidly* Good! Good! Gharbad will work. Gharbad will "
        "make you something special. Come back soon. "
        "Gharbad promises. Gharbad keeps promises!");
    add_choice(n, "I'll hold you to that.", -1);

    /* Path B: Threaten */
    n = add_node(ds, 705, NPC_ID_GHARBAD,
        "*cowers* No! No hurt! Gharbad has information! Valuable! "
        "The demons... they talk about a dark one below. "
        "A lord of terror. Gharbad heard the name... Diablo.");
    add_choice(n, "Tell me everything you know.", 706);
    c = add_choice(n, "[Draw weapon]", 707);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_GHARBAD, 0, -0.4f);
    add_consequence(c, CONSEQ_WORLD_EVENT, EVT_PLAYER_HARMED_NPC, NPC_ID_GHARBAD, 0.5f);

    n = add_node(ds, 706, NPC_ID_GHARBAD,
        "The demons gather strength on the lowest levels. They prepare "
        "for something. A ritual. Gharbad thinks they want to free "
        "their master. You must stop them!");
    c = add_choice(n, "Thank you, Gharbad.", -1);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_GHARBAD, 0, 0.1f);
    add_consequence(c, CONSEQ_GIVE_XP, 15, 0, 0);
    add_choice(n, "Useful. Stay out of my way.", -1);

    n = add_node(ds, 707, NPC_ID_GHARBAD,
        "*screams and falls to knees* PLEASE! Gharbad begs! "
        "Kill Gharbad and you lose everything Gharbad knows! "
        "Mercy! MERCY!");
    c = add_choice(n, "[Sheathe weapon] Fine. Talk.", 706);
    add_consequence(c, CONSEQ_MODIFY_TRUST, NPC_ID_GHARBAD, 0, 0.05f);
    c = add_choice(n, "[Walk away]", -1);
    add_consequence(c, CONSEQ_MODIFY_MOOD, NPC_ID_GHARBAD, 0, -0.3f);
}

void dialogue_data_init(DialogueSystem *ds)
{
    init_griswold(ds);
    init_pepin(ds);
    init_cain(ds);
    init_ogden(ds);
    init_farnham(ds);
    init_adria(ds);
    init_theo(ds);
    init_gharbad(ds);
}
