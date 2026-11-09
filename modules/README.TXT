About network module:
- user don't have to rename the needed layer into network.ldg
- selection is automatic, detection uses the cookie jar entries
- if user want to force the choice of layer, use FORCE_NETWORK_USE
  setting in the highwire.cfg
  
About the image decoders modules (imgd-*.ldg):
- externalized to reduce memory footprint, specific module is loaded
  if needed, since mime detection (or all if the format is unknown).
- user can disable a module by renaming its extension .ldg into .ldz
  such as now very rare xbm/xpm images, or avoid CPU consumption
  on low-end stock AtariST machine discarding jpg images.
- externalization makes easier future decoders additions and
  maintenance.
