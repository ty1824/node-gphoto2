/* Copyright contributors of the node-gphoto2 project */

#include <gphoto2/gphoto2-camera.h>
#include <stdio.h>
#include <string>
#include "binding.h"  // NOLINT

/**
 * This detects all currently attached cameras and returns
 * them in a list. It avoids the generic usb: entry.
 *
 * This function does not open nor initialize the cameras yet.
 */

int autodetect(CameraList *list, GPContext *context,
               GPPortInfoList **portinfolist,
               CameraAbilitiesList  **abilities) {
  int ret, i;
  CameraList *xlist = NULL;
  ret = gp_list_new(&xlist);
  if (ret < GP_OK) goto out;
  if (!*portinfolist) {
    /* Load all the port drivers we have... */
    ret = gp_port_info_list_new(portinfolist);
    if (ret < GP_OK) goto out;
    ret = gp_port_info_list_load(*portinfolist);
    if (ret < 0) goto out;
    ret = gp_port_info_list_count(*portinfolist);
    if (ret < 0) goto out;
  }
  /* Load all the camera drivers we have... */
  ret = gp_abilities_list_new(abilities);
  if (ret < GP_OK) goto out;
  ret = gp_abilities_list_load(*abilities, context);
  if (ret < GP_OK) goto out;

  /* ... and autodetect the currently attached cameras. */
  ret = gp_abilities_list_detect(*abilities, *portinfolist, xlist, context);

  if (ret < GP_OK) goto out;

  /* Filter out the "usb:" entry */
  ret = gp_list_count(xlist);

  if (ret < GP_OK) goto out;
  for (i = 0; i < ret; i++) {
    const char *name, *value;
    gp_list_get_name(xlist, i, &name);
    gp_list_get_value(xlist, i, &value);
    if (!strcmp("usb:", value)) continue;
    gp_list_append(list, name, value);
  }
  out:
    gp_list_free(xlist);
    return gp_list_count(list);
}

/**
 * This function opens a camera depending on the specified model and port.
 */

int open_camera(Camera **camera, std::string model, std::string port,
                GPPortInfoList *portinfolist, CameraAbilitiesList *abilities) {
  int ret, m, p;
  CameraAbilities a;
  GPPortInfo  pi;
  // printf("Opening camera using %p\n", camera);
  ret = gp_camera_new(camera);
  // printf("Created new camera handle %d, %p\n", ret, *camera);
  if (ret < GP_OK) return ret;

  /* First lookup the model / driver */
  m = gp_abilities_list_lookup_model(abilities, model.c_str());
  // printf("looked up model %d\n", m);
  if (m < GP_OK) return ret;
  ret = gp_abilities_list_get_abilities(abilities, m, &a);
  // printf("looked up abilities %d\n", ret);
  if (ret < GP_OK) return ret;
  ret = gp_camera_set_abilities(*camera, a);
  // printf("set abilities %d\n", ret);

  if (ret < GP_OK) return ret;

  /* Then associate the camera with the specified port */
  p = gp_port_info_list_lookup_path(portinfolist, port.c_str());
  if (ret < GP_OK) return ret;
  if (p == GP_ERROR_UNKNOWN_PORT) {
    fprintf(stderr, "The port you specified ('%s') can not be found. Please "
            "specify one of the ports found by 'gphoto2 --list-ports' and "
            "make sure the spelling is correct (i.e. with prefix 'serial:' "
            "or 'usb:').", port.c_str());
  }
  if (ret < GP_OK) return ret;
  ret = gp_port_info_list_get_info(portinfolist, p, &pi);
  // printf("port_info_list_get_info %d\n", ret);
  if (ret < GP_OK) return ret;
  ret = gp_camera_set_port_info(*camera, pi);
  // printf("gp_camera_set_port_info %d\n", ret);
  if (ret < GP_OK) return ret;
  // printf("open_camera finished %d\n", ret);
  return GP_OK;
}
