#include "config.h"
#include "eggsmclient-private.h"

#define EGG_TYPE_SM_CLIENT_DUMMY            (egg_sm_client_dummy_get_type ())
#define EGG_SM_CLIENT_DUMMY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_SM_CLIENT_DUMMY, EggSMClientDummy))
#define EGG_SM_CLIENT_DUMMY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EGG_TYPE_SM_CLIENT_DUMMY, EggSMClientDummyClass))
#define EGG_IS_SM_CLIENT_DUMMY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_SM_CLIENT_DUMMY))
#define EGG_IS_SM_CLIENT_DUMMY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EGG_TYPE_SM_CLIENT_DUMMY))
#define EGG_SM_CLIENT_DUMMY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), EGG_TYPE_SM_CLIENT_DUMMY, EggSMClientDummyClass))

typedef EggSMClient EggSMClientDummy;
typedef EggSMClientClass EggSMClientDummyClass;

G_DEFINE_TYPE (EggSMClientDummy, egg_sm_client_dummy, EGG_TYPE_SM_CLIENT)

static void
egg_sm_client_dummy_init (G_GNUC_UNUSED EggSMClientDummy *client)
{
}

static void
sm_client_dummy_startup (G_GNUC_UNUSED EggSMClient *client,
			 G_GNUC_UNUSED const char *client_id)
{
}

static void
egg_sm_client_dummy_class_init (EggSMClientDummyClass *klass)
{
  klass->startup = sm_client_dummy_startup;
}

EggSMClient *
egg_sm_client_dummy_new (void)
{
  return g_object_new (EGG_TYPE_SM_CLIENT_DUMMY, NULL);
}
